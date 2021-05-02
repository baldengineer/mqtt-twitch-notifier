## MQTT Notifier used for Twitch Streams

An ESP8266 based notifier. It connects to a private MQTT broker and subscribes to several topics. A [a python-based Twitch bot](https://github.com/tisboyo/Twitch_Bot) posts messages based on events happening in the live stream.

Hardware includes:
* Adafruit HUZZAH ESP8266
* SparkFun Qwuiic MP3 Player
* A Speaker

Earlier revisions included some NeoPixels. Currently they are not being used, but may return.

### What about the sounds?!
I have some sounds I created from file I found on the internet. You need to put the mp3s on the SD Card with file names shown in the code. The code/player calls them tracks.

## Live Stream
* [Bald Engineer Live Stream](https://twitch.tv/baldengineer)