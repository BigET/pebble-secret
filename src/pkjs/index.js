Pebble.addEventListener('ready', function() {console.log('PebbleKit JS ready!');});

Pebble.addEventListener('showConfiguration', function() {
    var url = 'https://rawgit.com/BigET/pebble-secret/master/config/index.html';
    console.log('Showing configuration page: ' + url);

    Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
    var configData = JSON.parse(decodeURIComponent(e.response));
    console.log('Configuration page returned: ' + JSON.stringify(configData));

    var dict = {
        "SecretTitle":configData["SecretTitle"],
        "SecretText":configData["SecretText"],
    };
    Pebble.sendAppMessage(dict,
        function() {console.log('Send successful: ' + JSON.stringify(dict));},
        function() {console.log('Send failed!');});
});
