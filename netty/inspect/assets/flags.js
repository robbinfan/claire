var awaitingResponse = false; // Whether there is currently an outbound request.

/**
 * bind method to keyup change event
 */
function bindEvent() {
    $('.new_value').bind('keyup change', inputChanged());
}


/**
 * keyup change callback, change assiociated check attr
 */
function inputChanged() {
    return function() {
        var value = $(this).val();
        var tr = $(this).parent().parent();
        var td = $("#modify", tr);
        var checkbox = $("input", td);
        checkbox.attr("checked", value.length > 0);
        checkbox.change();
    }
}


/**
 * Display error message in error panel.
 * @param {string} message Message to display in panel.
 */
function error(message) {
    $('<div>').appendTo($('#error-messages')).html('<pre>' + message + '</pre>');
}


/**
 * Display request errors in error panel.
 * @param {object} XMLHttpRequest object.
 */
function handleRequestError(response) {
    var contentType = response.getResponseHeader('content-type');
    if (contentType == 'application/json') {
        var response_error = $.parseJSON(response.responseText);
        var error_message = response_error.error_message;
        if (error.state == 'APPLICATION_ERROR' && error.error_name) {
            error_message = error_message + ' (' + error.error_name + ')';
        }
    } else {
        error_message = '' + response.status + ': ' + response.statusText;
    }

    error(error_message);
}


/**
 * Send JSON RPC to remote.
 * @param {string} path Path of service on originating server to send request.
 * @param {Object} request Message to send as request.
 * @param {function} on_success Function to call upon successful request.
 */
function sendRequest(path, request, onSuccess) {
    $.ajax({
        url : path,
        type: 'POST',
        contentType: 'application/json',
        data: $.toJSON(request),
        success: onSuccess,
        error: handleRequestError
    });
}


/**
 * Extract entire message from each modifyed flag row.
 * @return {Object} Fully populated message object ready to transmit
 *     as JSON message.
 */
function fromFlagRow() {
    var message = {}
    $("tr").each(function(i, tr) {
        var name = $("td.name", tr).text();
        var checked = $("td input:checkbox", tr).attr("checked");
        var new_value = $.trim($("td input:text", tr).val());
        var current_value = $("td.current_value", tr).text();
        if (checked) {
            message[name] = new_value;
        }
    });

    return message;
}

/**
 * Apply all modify to remote server
 */
function applyModify() {
    if (awaitingResponse)
    {
        return ;
    }
    awaitingResponse = true;

    $('#error-messages').empty();
    request = fromFlagRow();
    if ($.isEmptyObject(request)) {
        return;
    }
    sendRequest(
        "/flags",
        request,
        function(response){
            for (var obj in request) {
                $("tr").each(function(i, tr) {
                    if ($("td.name", tr).text() == obj) {
                        $(this).attr("style", "color:green");
                        $("td.current_value", tr).text($.trim($("td input:text", tr).val()));
                        $("td input:text", tr).val("");
                        $("td input:checkbox", tr).attr("checked", false);
                        $("td input:checkbox", tr).change();
                    }
                });
            }

            for (var obj in response) {
                error(obj + "modify to " + response[obj] + " failed");
                $("tr").each(function(i, tr) {
                    if ($("td.name", tr).text() == obj) {
                        $(this).attr("style", "color:red");
                    }
                });
            }
            awaitingResponse = false;
        });
}
