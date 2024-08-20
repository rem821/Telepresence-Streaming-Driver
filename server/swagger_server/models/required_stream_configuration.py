# coding: utf-8

from __future__ import absolute_import
from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from swagger_server.models.base_model_ import Model
from swagger_server.models.apiv1streamupdate_resolution import Apiv1streamupdateResolution  # noqa: F401,E501
from swagger_server.models.stream_configuration import StreamConfiguration  # noqa: F401,E501
from swagger_server import util


class RequiredStreamConfiguration(Model):
    """NOTE: This class is auto generated by the swagger code generator program.

    Do not edit the class manually.
    """
    def __init__(self, ip_address: str=None, port: int=None, codec: str=None, bitrate: str=None, resolution: Apiv1streamupdateResolution=None, video_mode: str=None, fps: int=None):  # noqa: E501
        """RequiredStreamConfiguration - a model defined in Swagger

        :param ip_address: The ip_address of this RequiredStreamConfiguration.  # noqa: E501
        :type ip_address: str
        :param port: The port of this RequiredStreamConfiguration.  # noqa: E501
        :type port: int
        :param codec: The codec of this RequiredStreamConfiguration.  # noqa: E501
        :type codec: str
        :param bitrate: The bitrate of this RequiredStreamConfiguration.  # noqa: E501
        :type bitrate: str
        :param resolution: The resolution of this RequiredStreamConfiguration.  # noqa: E501
        :type resolution: Apiv1streamupdateResolution
        :param video_mode: The video_mode of this RequiredStreamConfiguration.  # noqa: E501
        :type video_mode: str
        :param fps: The fps of this RequiredStreamConfiguration.  # noqa: E501
        :type fps: int
        """
        self.swagger_types = {
            'ip_address': str,
            'port': int,
            'codec': str,
            'bitrate': str,
            'resolution': Apiv1streamupdateResolution,
            'video_mode': str,
            'fps': int
        }

        self.attribute_map = {
            'ip_address': 'ip_address',
            'port': 'port',
            'codec': 'codec',
            'bitrate': 'bitrate',
            'resolution': 'resolution',
            'video_mode': 'video_mode',
            'fps': 'fps'
        }
        self._ip_address = ip_address
        self._port = port
        self._codec = codec
        self._bitrate = bitrate
        self._resolution = resolution
        self._video_mode = video_mode
        self._fps = fps

    @classmethod
    def from_dict(cls, dikt) -> 'RequiredStreamConfiguration':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The RequiredStreamConfiguration of this RequiredStreamConfiguration.  # noqa: E501
        :rtype: RequiredStreamConfiguration
        """
        return util.deserialize_model(dikt, cls)

    @property
    def ip_address(self) -> str:
        """Gets the ip_address of this RequiredStreamConfiguration.


        :return: The ip_address of this RequiredStreamConfiguration.
        :rtype: str
        """
        return self._ip_address

    @ip_address.setter
    def ip_address(self, ip_address: str):
        """Sets the ip_address of this RequiredStreamConfiguration.


        :param ip_address: The ip_address of this RequiredStreamConfiguration.
        :type ip_address: str
        """
        if ip_address is None:
            raise ValueError("Invalid value for `ip_address`, must not be `None`")  # noqa: E501

        self._ip_address = ip_address

    @property
    def port(self) -> int:
        """Gets the port of this RequiredStreamConfiguration.


        :return: The port of this RequiredStreamConfiguration.
        :rtype: int
        """
        return self._port

    @port.setter
    def port(self, port: int):
        """Sets the port of this RequiredStreamConfiguration.


        :param port: The port of this RequiredStreamConfiguration.
        :type port: int
        """
        if port is None:
            raise ValueError("Invalid value for `port`, must not be `None`")  # noqa: E501

        self._port = port

    @property
    def codec(self) -> str:
        """Gets the codec of this RequiredStreamConfiguration.


        :return: The codec of this RequiredStreamConfiguration.
        :rtype: str
        """
        return self._codec

    @codec.setter
    def codec(self, codec: str):
        """Sets the codec of this RequiredStreamConfiguration.


        :param codec: The codec of this RequiredStreamConfiguration.
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
    def bitrate(self) -> str:
        """Gets the bitrate of this RequiredStreamConfiguration.


        :return: The bitrate of this RequiredStreamConfiguration.
        :rtype: str
        """
        return self._bitrate

    @bitrate.setter
    def bitrate(self, bitrate: str):
        """Sets the bitrate of this RequiredStreamConfiguration.


        :param bitrate: The bitrate of this RequiredStreamConfiguration.
        :type bitrate: str
        """
        if bitrate is None:
            raise ValueError("Invalid value for `bitrate`, must not be `None`")  # noqa: E501

        self._bitrate = bitrate

    @property
    def resolution(self) -> Apiv1streamupdateResolution:
        """Gets the resolution of this RequiredStreamConfiguration.


        :return: The resolution of this RequiredStreamConfiguration.
        :rtype: Apiv1streamupdateResolution
        """
        return self._resolution

    @resolution.setter
    def resolution(self, resolution: Apiv1streamupdateResolution):
        """Sets the resolution of this RequiredStreamConfiguration.


        :param resolution: The resolution of this RequiredStreamConfiguration.
        :type resolution: Apiv1streamupdateResolution
        """
        if resolution is None:
            raise ValueError("Invalid value for `resolution`, must not be `None`")  # noqa: E501

        self._resolution = resolution

    @property
    def video_mode(self) -> str:
        """Gets the video_mode of this RequiredStreamConfiguration.


        :return: The video_mode of this RequiredStreamConfiguration.
        :rtype: str
        """
        return self._video_mode

    @video_mode.setter
    def video_mode(self, video_mode: str):
        """Sets the video_mode of this RequiredStreamConfiguration.


        :param video_mode: The video_mode of this RequiredStreamConfiguration.
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
        """Gets the fps of this RequiredStreamConfiguration.


        :return: The fps of this RequiredStreamConfiguration.
        :rtype: int
        """
        return self._fps

    @fps.setter
    def fps(self, fps: int):
        """Sets the fps of this RequiredStreamConfiguration.


        :param fps: The fps of this RequiredStreamConfiguration.
        :type fps: int
        """
        if fps is None:
            raise ValueError("Invalid value for `fps`, must not be `None`")  # noqa: E501

        self._fps = fps
