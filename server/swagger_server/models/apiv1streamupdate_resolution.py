# coding: utf-8

from __future__ import absolute_import
from datetime import date, datetime  # noqa: F401

from typing import List, Dict  # noqa: F401

from swagger_server.models.base_model_ import Model
from swagger_server import util


class Apiv1streamupdateResolution(Model):
    """NOTE: This class is auto generated by the swagger code generator program.

    Do not edit the class manually.
    """
    def __init__(self, width: int=None, height: int=None):  # noqa: E501
        """Apiv1streamupdateResolution - a model defined in Swagger

        :param width: The width of this Apiv1streamupdateResolution.  # noqa: E501
        :type width: int
        :param height: The height of this Apiv1streamupdateResolution.  # noqa: E501
        :type height: int
        """
        self.swagger_types = {
            'width': int,
            'height': int
        }

        self.attribute_map = {
            'width': 'width',
            'height': 'height'
        }
        self._width = width
        self._height = height

    @classmethod
    def from_dict(cls, dikt) -> 'Apiv1streamupdateResolution':
        """Returns the dict as a model

        :param dikt: A dict.
        :type: dict
        :return: The apiv1streamupdate_resolution of this Apiv1streamupdateResolution.  # noqa: E501
        :rtype: Apiv1streamupdateResolution
        """
        return util.deserialize_model(dikt, cls)

    @property
    def width(self) -> int:
        """Gets the width of this Apiv1streamupdateResolution.


        :return: The width of this Apiv1streamupdateResolution.
        :rtype: int
        """
        return self._width

    @width.setter
    def width(self, width: int):
        """Sets the width of this Apiv1streamupdateResolution.


        :param width: The width of this Apiv1streamupdateResolution.
        :type width: int
        """

        self._width = width

    @property
    def height(self) -> int:
        """Gets the height of this Apiv1streamupdateResolution.


        :return: The height of this Apiv1streamupdateResolution.
        :rtype: int
        """
        return self._height

    @height.setter
    def height(self, height: int):
        """Sets the height of this Apiv1streamupdateResolution.


        :param height: The height of this Apiv1streamupdateResolution.
        :type height: int
        """

        self._height = height
