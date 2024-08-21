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

# This object  represent the current state and is mutated by the setter endpoints
stream_state = StreamState(False, "192.168.1.100", 8554, "JPEG", "400k",
                           Apiv1streamupdateResolution(1920, 1080), "stereo", 30)


def api_v1_stream_start_post(body):  # noqa: E501
    """Start the video streaming with configuration.

     # noqa: E501

    :param body: 
    :type body: dict | bytes

    :rtype: InlineResponse200
    """
    global stream_state

    if stream_state.is_streaming:
        return "Stream already running!"

    if connexion.request.is_json:
        body = RequiredStreamConfiguration.from_dict(connexion.request.get_json())  # noqa: E501
        print(body)
        stream_state = StreamState(True, body.ip_address, body.port, body.codec, body.bitrate, body.resolution,
                                   body.video_mode, body.fps)
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
    global stream_state

    if not stream_state.is_streaming:
        return "Stream already stopped!"
    stream_state.is_streaming = False

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
        stream_state = StreamState(stream_state.is_streaming, body.ip_address, body.port, body.codec, body.bitrate, body.resolution,
                                   body.video_mode, body.fps)
    else:
        return "Missing body!"

    return stream_state
