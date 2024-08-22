import subprocess
import threading
from itertools import filterfalse

import connexion
import six
from connexion import Resolution
from setuptools import Require

from swagger_server.models import StreamConfiguration, Apiv1streamupdateResolution
from swagger_server.models.inline_response200 import InlineResponse200  # noqa: E501
from swagger_server.models.inline_response2001 import InlineResponse2001  # noqa: E501
from swagger_server.models.inline_response2002 import InlineResponse2002  # noqa: E501
from swagger_server.models.inline_response500 import InlineResponse500  # noqa: E501
from swagger_server.models.inline_response5001 import InlineResponse5001  # noqa: E501
from swagger_server.models.inline_response5002 import InlineResponse5002  # noqa: E501
from swagger_server.models.inline_response5003 import InlineResponse5003  # noqa: E501
from swagger_server.models.required_stream_configuration import RequiredStreamConfiguration  # noqa: E501
from swagger_server.models.stream_state import StreamState  # noqa: E501
from swagger_server.models.stream_update_body import StreamUpdateBody  # noqa: E501
from swagger_server import util

# This object represents the current state and is mutated by the setter endpoints
stream_state = StreamState(False, "192.168.1.100", 8554, "JPEG", "400k",
                           Apiv1streamupdateResolution(1920, 1080), "stereo", 30)

exec_path = "/home/standa/Desktop/Projekty/Telepresence-Streaming-Driver/cmake-build-debug/telepresence_streaming_driver"
process = None
streaming_thread = None

def run_streaming_process():
    global stream_state, process
    print("Starting streaming process!")

    stream_state.is_streaming = True

    process = subprocess.Popen(
        [exec_path, "first_arg"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,  # Ensures the output is in string format rather than bytes
        bufsize=1,  # Line-buffered output
    )

    # Read stdout line by line in real-time
    for stdout_line in iter(process.stdout.readline, ""):
        print(stdout_line, end="")  # Print each line as it is received

    # Wait for the process to finish and capture any remaining output
    process.stdout.close()  # Close stdout
    process.wait()

    # Handle stderr
    stderr = process.stderr.read()
    if stderr:
        print(f'[stderr]\n{stderr}')

    stream_state.is_streaming = False
    print("The streaming process has ended")


def api_v1_stream_start_post(body):  # noqa: E501
    """Start the video streaming with configuration.

     # noqa: E501

    :param body: 
    :type body: dict | bytes

    :rtype: InlineResponse200
    """
    global stream_state, streaming_thread, process

    if stream_state.is_streaming:
        return "Stream already running!"

    if connexion.request.is_json:
        body = RequiredStreamConfiguration.from_dict(connexion.request.get_json())  # noqa: E501
        print(body)
        stream_state = StreamState(False, body.ip_address, body.port, body.codec, body.bitrate, body.resolution,
                                   body.video_mode, body.fps)

        streaming_thread = threading.Thread(target=run_streaming_process)
        streaming_thread.start()

    else:
        return "Missing body!"

    return stream_state


def api_v1_stream_state_get():  # noqa: E501
    """Get the current state of the video streaming.

     # noqa: E501


    :rtype: StreamState
    """
    global stream_state

    return stream_state


def api_v1_stream_stop_post():  # noqa: E501
    """Stop the video streaming.

     # noqa: E501


    :rtype: InlineResponse2001
    """
    global stream_state, streaming_thread, process

    if not stream_state.is_streaming:
        return "Stream already stopped!"

    process.terminate()
    streaming_thread.join()
    print("The streaming process has been killed!")

    return stream_state


def api_v1_stream_update_put(body):  # noqa: E501
    """Update the stream configuration dynamically.

     # noqa: E501

    :param body: 
    :type body: dict | bytes

    :rtype: InlineResponse2002
    """
    global stream_state

    if connexion.request.is_json:
        body = StreamUpdateBody.from_dict(connexion.request.get_json())  # noqa: E501
        print(body)
        stream_state = StreamState(stream_state.is_streaming, body.ip_address, body.port, body.codec, body.bitrate,
                                   body.resolution,
                                   body.video_mode, body.fps)
    else:
        return "Missing body!"

    return stream_state
