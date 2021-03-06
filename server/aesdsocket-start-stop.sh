#! /bin/sh

case "$1" in
        start)
                echo "Starting aesdsocket"
                start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
                ;;

        stop)
                echo "handle SIGTERM"
                start-stop-daemon -K -n aesdsocket --signal TERM
                ;;
        *)
                echo "Usage : $0 {start|stop}"
        exit 1
esac
