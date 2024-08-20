import connexion
import six

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


def api_v1_stream_start_post(body):  # noqa: E501
    """Start the video streaming with configuration.

     # noqa: E501

    :param body: 
    :type body: dict | bytes

    :rtype: InlineResponse200
    """
    if connexion.request.is_json:
        body = RequiredStreamConfiguration.from_dict(connexion.request.get_json())  # noqa: E501
        print(body)
    return 'do some magic!'


def api_v1_stream_state_get():  # noqa: E501
    """Get the current state of the video streaming.

     # noqa: E501


    :rtype: StreamState
    """
    return print("get called!")


def api_v1_stream_stop_post():  # noqa: E501
    """Stop the video streaming.

     # noqa: E501


    :rtype: InlineResponse2001
    """
    return print("stop called!")


def api_v1_stream_update_put(body):  # noqa: E501
    """Update the stream configuration dynamically.

     # noqa: E501

    :param body: 
    :type body: dict | bytes

    :rtype: InlineResponse2002
    """
    if connexion.request.is_json:
        body = StreamUpdateBody.from_dict(connexion.request.get_json())  # noqa: E501
        print(body)
    return 'do some magic!'
