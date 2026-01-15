"""
Main UDP relay service for robot control.

This service receives commands via UDP, routes them to the appropriate
destination (servo driver or robot), and returns responses.
"""

import logging
import signal
import socket
import sys
from typing import Optional, Tuple

from .config import load_configuration, RelayConfig
from .exceptions import RelayServiceError
from .protocol import MessageDetector, MessageType
from .servo_translators import ServoTranslator, TGDrivesTranslator


class UDPRelayService:
    """
    Main UDP relay service.

    Receives UDP messages on ingest port, routes them based on message type:
    - Head pose messages (0x01 prefix) -> servo translator -> servo driver
    - Robot control messages (0x02 prefix) -> robot controller
    """

    def __init__(self, config: RelayConfig):
        """
        Initialize the UDP relay service.

        Args:
            config: Service configuration
        """
        self.config = config
        self.logger = logging.getLogger(__name__)

        # Sockets
        self.ingest_socket: Optional[socket.socket] = None
        self.robot_socket: Optional[socket.socket] = None

        # Protocol components
        self.message_detector = MessageDetector()

        # Servo translator (initialized based on config)
        self.servo_translator: Optional[ServoTranslator] = None

        # State
        self.running = False
        self.consecutive_errors = 0

    def _create_servo_translator(self) -> ServoTranslator:
        """
        Create servo translator based on configuration.

        Returns:
            Servo translator instance

        Raises:
            RelayServiceError: If translator type is unknown
        """
        translator_type = self.config.servo_translator.lower()

        if translator_type == 'tg_drives':
            return TGDrivesTranslator(
                self.config.servo_ip,
                self.config.servo_port,
                self.config.servo_response_timeout,
                azimuth_min=self.config.tg_azimuth_min,
                azimuth_max=self.config.tg_azimuth_max,
                elevation_min=self.config.tg_elevation_min,
                elevation_max=self.config.tg_elevation_max,
                speed_max=self.config.tg_speed_max,
                speed_multiplier=self.config.tg_speed_multiplier,
                filter_alpha=self.config.tg_filter_alpha,
                swap_axes=self.config.tg_swap_axes
            )
        else:
            raise RelayServiceError(f"Unknown servo translator type: {translator_type}")

    def _initialize_sockets(self) -> bool:
        """
        Initialize UDP sockets.

        Returns:
            True if all sockets initialized successfully, False otherwise
        """
        try:
            # Create ingest socket
            self.ingest_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.ingest_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.ingest_socket.bind((self.config.ingest_host, self.config.ingest_port))
            self.logger.info(f"Ingest socket bound to {self.config.ingest_host}:{self.config.ingest_port}")

            # Create robot socket
            self.robot_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.robot_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.robot_socket.bind(("0.0.0.0", 0))  # Bind to any available port
            self.robot_socket.settimeout(self.config.robot_response_timeout)
            self.logger.info("Robot socket initialized")

            # Initialize servo translator
            self.servo_translator = self._create_servo_translator()
            if not self.servo_translator.initialize():
                self.logger.error("Failed to initialize servo translator")
                return False

            self.logger.info("All sockets initialized successfully")
            return True

        except Exception as e:
            self.logger.error(f"Failed to initialize sockets: {e}")
            self._close_sockets()
            return False

    def _close_sockets(self):
        """Close all sockets."""
        if self.ingest_socket:
            try:
                self.ingest_socket.close()
            except Exception as e:
                self.logger.warning(f"Error closing ingest socket: {e}")
            self.ingest_socket = None

        if self.robot_socket:
            try:
                self.robot_socket.close()
            except Exception as e:
                self.logger.warning(f"Error closing robot socket: {e}")
            self.robot_socket = None

        if self.servo_translator:
            try:
                self.servo_translator.close()
            except Exception as e:
                self.logger.warning(f"Error closing servo translator: {e}")
            self.servo_translator = None

    def _route_message(self, data: bytes, client_addr: Tuple[str, int]):
        """
        Route message to appropriate destination.

        Args:
            data: Raw message data
            client_addr: Client address (IP, port)
        """
        # Detect message type
        message_type = self.message_detector.detect_message_type(data)

        if message_type == MessageType.HEAD_POSE:
            self._forward_to_servo(data, client_addr)

        elif message_type == MessageType.ROBOT_CONTROL:
            self._forward_to_robot(data, client_addr)

        else:
            self.logger.warning(f"Unknown message type from {client_addr[0]}:{client_addr[1]}, dropping")

    def _forward_to_servo(self, data: bytes, client_addr: Tuple[str, int]):
        """
        Forward message to servo driver via translator.

        Args:
            data: Servo command data
            client_addr: Client address to send response to
        """
        if not self.servo_translator:
            self.logger.error("Servo translator not initialized")
            return

        try:
            # Translator handles protocol conversion, sending, and receiving
            response = self.servo_translator.translate_and_forward(data, client_addr)

            # Send response back to client if we got one
            if response and self.ingest_socket:
                self.ingest_socket.sendto(response, client_addr)
                self.logger.debug(f"Response sent back to {client_addr[0]}:{client_addr[1]}")

            # Reset error counter on success
            self.consecutive_errors = 0

        except Exception as e:
            self.consecutive_errors += 1
            self.logger.error(f"Error forwarding to servo: {e}")

    def _forward_to_robot(self, data: bytes, client_addr: Tuple[str, int]):
        """
        Forward message to robot controller.

        Args:
            data: Robot command data
            client_addr: Client address (not used for robot commands)
        """
        if not self.robot_socket:
            self.logger.error("Robot socket not initialized")
            return

        try:
            self.robot_socket.sendto(data, (self.config.robot_ip, self.config.robot_port))
            self.logger.debug(f"Forwarded robot command to {self.config.robot_ip}:{self.config.robot_port}")

            # Robot commands typically don't expect responses
            # If they do in the future, add response handling here

            # Reset error counter on success
            self.consecutive_errors = 0

        except Exception as e:
            self.consecutive_errors += 1
            self.logger.error(f"Error forwarding to robot: {e}")

    def _listen_loop(self):
        """
        Main message receiving loop.

        Receives messages from ingest socket and routes them.
        """
        self.logger.info("Entering main listen loop")

        while self.running:
            try:
                # Receive message
                data, client_addr = self.ingest_socket.recvfrom(self.config.socket_buffer_size)

                self.logger.debug(f"Received {len(data)} bytes from {client_addr[0]}:{client_addr[1]}")

                # Route message
                self._route_message(data, client_addr)

                # Check for too many consecutive errors
                if self.consecutive_errors >= self.config.max_consecutive_errors:
                    self.logger.error(
                        f"Too many consecutive errors ({self.consecutive_errors}), stopping service"
                    )
                    self.running = False
                    break

            except KeyboardInterrupt:
                self.logger.info("Keyboard interrupt received")
                self.running = False
                break

            except Exception as e:
                self.consecutive_errors += 1
                self.logger.error(f"Error in listen loop: {e}", exc_info=True)

                if self.consecutive_errors >= self.config.max_consecutive_errors:
                    self.logger.error(
                        f"Too many consecutive errors ({self.consecutive_errors}), stopping service"
                    )
                    self.running = False
                    break

        self.logger.info("Exited main listen loop")

    def start(self):
        """
        Start the relay service.

        This method blocks until the service is stopped.
        """
        self.logger.info("Starting UDP Relay Service")
        self.logger.info(f"Configuration: {self.config}")

        # Initialize sockets
        if not self._initialize_sockets():
            self.logger.error("Failed to initialize sockets, exiting")
            raise RelayServiceError("Socket initialization failed")

        # Set running flag
        self.running = True

        # Enter main loop
        try:
            self._listen_loop()
        finally:
            self.stop()

    def stop(self):
        """Stop the relay service and clean up resources."""
        self.logger.info("Stopping relay service")
        self.running = False

        # Close all sockets
        self._close_sockets()

        self.logger.info("Relay service stopped")


# Global service instance for signal handling
_service_instance: Optional[UDPRelayService] = None


def signal_handler(signum, frame):
    """Handle termination signals for graceful shutdown."""
    logger = logging.getLogger(__name__)
    logger.info(f"Received signal {signum}, initiating shutdown")

    global _service_instance
    if _service_instance:
        _service_instance.stop()


def setup_logging(log_level: str, log_file: Optional[str] = None):
    """
    Setup logging configuration.

    Args:
        log_level: Logging level (DEBUG, INFO, WARNING, ERROR)
        log_file: Optional log file path
    """
    level = getattr(logging, log_level.upper())

    # Create formatter
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    # Setup root logger
    logger = logging.getLogger()
    logger.setLevel(level)

    # Remove any existing handlers
    logger.handlers = []

    # Console handler
    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setLevel(level)
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)

    # File handler if specified
    if log_file:
        file_handler = logging.FileHandler(log_file)
        file_handler.setLevel(level)
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)


def main():
    """Main entry point for the relay service."""
    try:
        # Load configuration
        config = load_configuration()

        # Setup logging
        setup_logging(config.log_level, config.log_file)

        logger = logging.getLogger(__name__)
        logger.info("BUT Telepresence Robot Control Relay Service")

        # Setup signal handlers
        signal.signal(signal.SIGTERM, signal_handler)
        signal.signal(signal.SIGINT, signal_handler)

        # Create and start service
        global _service_instance
        _service_instance = UDPRelayService(config)
        _service_instance.start()

        return 0

    except KeyboardInterrupt:
        logging.info("Keyboard interrupt received")
        return 0

    except Exception as e:
        logging.error(f"Fatal error: {e}", exc_info=True)
        return 1


if __name__ == "__main__":
    sys.exit(main())
