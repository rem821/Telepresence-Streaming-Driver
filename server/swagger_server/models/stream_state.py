# coding: utf-8

from __future__ import absolute_import
from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from swagger_server.models.base_model_ import Model
from swagger_server.models.apiv1streamupdate_resolution import Apiv1streamupdateResolution  # noqa: F401,E501
from swagger_server.models.stream_configuration import StreamConfiguration  # noqa: F401,E501
from swagger_server import util


class StreamState(Model):
    """NOTE: This class is auto generated by the swagger code generator program.

    Do not edit the class manually.
    """
    def __init__(self, is_streaming: bool=None, ip_address: str=None, port_left: int=None, port_right: int=None, codec: str=None, encoding_quality: int=None, bitrate: str=None, resolution: Apiv1streamupdateResolution=None, video_mode: str=None, fps: int=None):  # noqa: E501
        """StreamState - a model defined in Swagger

        :param is_streaming: The is_streaming of this StreamState.  # noqa: E501
        :type is_streaming: bool
        :param ip_address: The ip_address of this StreamState.  # noqa: E501
        :type ip_address: str
        :param port_left: The port_left of this StreamState.  # noqa: E501
        :type port_left: int
        :param port_right: The port_right of this StreamState.  # noqa: E501
        :type port_right: int
        :param codec: The codec of this StreamState.  # noqa: E501
        :type codec: str
        :param encoding_quality: The encoding_quality of this StreamState.  # noqa: E501
        :type encoding_quality: int
        :param bitrate: The bitrate of this StreamState.  # noqa: E501
        :type bitrate: str
        :param resolution: The resolution of this StreamState.  # noqa: E501
        :type resolution: Apiv1streamupdateResolution
        :param video_mode: The video_mode of this StreamState.  # noqa: E501
        :type video_mode: str
        :param fps: The fps of this StreamState.  # noqa: E501
        :type fps: int
        """
        self.swagger_types = {
            'is_streaming': bool,
            'ip_address': str,
            'port_left': int,
            'port_right': int,
            'codec': str,
            'encoding_quality': int,
            'bitrate': str,
            'resolution': Apiv1streamupdateResolution,
            'video_mode': str,
            'fps': int
        }

        self.attribute_map = {
            'is_streaming': 'is_streaming',
            'ip_address': 'ip_address',
            'port_left': 'port_left',
            'port_right': 'port_right',
            'codec': 'codec',
            'encoding_quality': 'encoding_quality',
            'bitrate': 'bitrate',
            'resolution': 'resolution',
            'video_mode': 'video_mode',
            'fps': 'fps'
        }
        self._is_streaming = is_streaming
        self._ip_address = ip_address
        self._port_left = port_left
        self._port_right = port_right
        self._codec = codec
        self._encoding_quality = encoding_quality
        self._bitrate = bitrate
        self._resolution = resolution
        self._video_mode = video_mode
        self._fps = fps

    @classmethod
    def from_dict(cls, dikt) -> 'StreamState':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The StreamState of this StreamState.  # noqa: E501
        :rtype: StreamState
        """
        return util.deserialize_model(dikt, cls)

    @property
    def is_streaming(self) -> bool:
        """Gets the is_streaming of this StreamState.


        :return: The is_streaming of this StreamState.
        :rtype: bool
        """
        return self._is_streaming

    @is_streaming.setter
    def is_streaming(self, is_streaming: bool):
        """Sets the is_streaming of this StreamState.


        :param is_streaming: The is_streaming of this StreamState.
        :type is_streaming: bool
        """

        self._is_streaming = is_streaming

    @property
    def ip_address(self) -> str:
        """Gets the ip_address of this StreamState.


        :return: The ip_address of this StreamState.
        :rtype: str
        """
        return self._ip_address

    @ip_address.setter
    def ip_address(self, ip_address: str):
        """Sets the ip_address of this StreamState.


        :param ip_address: The ip_address of this StreamState.
        :type ip_address: str
        """

        self._ip_address = ip_address

    @property
    def port_left(self) -> int:
        """Gets the port_left of this StreamState.


        :return: The port_left of this StreamState.
        :rtype: int
        """
        return self._port_left

    @port_left.setter
    def port_left(self, port_left: int):
        """Sets the port_left of this StreamState.


        :param port_left: The port_left of this StreamState.
        :type port_left: int
        """

        self._port_left = port_left

    @property
    def port_right(self) -> int:
        """Gets the port_right of this StreamState.


        :return: The port_right of this StreamState.
        :rtype: int
        """
        return self._port_right

    @port_right.setter
    def port_right(self, port_right: int):
        """Sets the port_right of this StreamState.


        :param port_right: The port_right of this StreamState.
        :type port_right: int
        """

        self._port_right = port_right

    @property
    def codec(self) -> str:
        """Gets the codec of this StreamState.


        :return: The codec of this StreamState.
        :rtype: str
        """
        return self._codec

    @codec.setter
    def codec(self, codec: str):
        """Sets the codec of this StreamState.


        :param codec: The codec of this StreamState.
        :type codec: str
        """
        allowed_values = ["H264", "H265", "VP8", "VP9", "JPEG"]  # noqa: E501
        if codec not in allowed_values:
            raise ValueError(
                "Invalid value for `codec` ({0}), must be one of {1}"
                .format(codec, allowed_values)
            )

        self._codec = codec

    @property
    def encoding_quality(self) -> int:
        """Gets the encoding_quality of this StreamState.


        :return: The encoding_quality of this StreamState.
        :rtype: int
        """
        return self._encoding_quality

    @encoding_quality.setter
    def encoding_quality(self, encoding_quality: int):
        """Sets the encoding_quality of this StreamState.


        :param encoding_quality: The encoding_quality of this StreamState.
        :type encoding_quality: int
        """

        self._encoding_quality = encoding_quality

    @property
    def bitrate(self) -> str:
        """Gets the bitrate of this StreamState.


        :return: The bitrate of this StreamState.
        :rtype: str
        """
        return self._bitrate

    @bitrate.setter
    def bitrate(self, bitrate: str):
        """Sets the bitrate of this StreamState.


        :param bitrate: The bitrate of this StreamState.
        :type bitrate: str
        """

        self._bitrate = bitrate

    @property
    def resolution(self) -> Apiv1streamupdateResolution:
        """Gets the resolution of this StreamState.


        :return: The resolution of this StreamState.
        :rtype: Apiv1streamupdateResolution
        """
        return self._resolution

    @resolution.setter
    def resolution(self, resolution: Apiv1streamupdateResolution):
        """Sets the resolution of this StreamState.


        :param resolution: The resolution of this StreamState.
        :type resolution: Apiv1streamupdateResolution
        """

        self._resolution = resolution

    @property
    def video_mode(self) -> str:
        """Gets the video_mode of this StreamState.


        :return: The video_mode of this StreamState.
        :rtype: str
        """
        return self._video_mode

    @video_mode.setter
    def video_mode(self, video_mode: str):
        """Sets the video_mode of this StreamState.


        :param video_mode: The video_mode of this StreamState.
        :type video_mode: str
        """
        allowed_values = ["stereo", "mono"]  # noqa: E501
        if video_mode not in allowed_values:
            raise ValueError(
                "Invalid value for `video_mode` ({0}), must be one of {1}"
                .format(video_mode, allowed_values)
            )

        self._video_mode = video_mode

    @property
    def fps(self) -> int:
        """Gets the fps of this StreamState.


        :return: The fps of this StreamState.
        :rtype: int
        """
        return self._fps

    @fps.setter
    def fps(self, fps: int):
        """Sets the fps of this StreamState.


        :param fps: The fps of this StreamState.
        :type fps: int
        """

        self._fps = fps
