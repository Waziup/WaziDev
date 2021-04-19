#Port where the WaziDev is connected
PORT=/dev/ttyUSB0
#LoRAWAN device address
DEVADDR="26011D00"
#temperature value to send via LoRaWAN
VAL="61"

#Compilation and upload
make
sudo make upload

#Configure the port
stty -F $PORT cs8 38400 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts

#Monitor the Serial port
tail -f $PORT &

sleep 3
# Send Dev Addr
echo $DEVADDR > $PORT

sleep 1
# Send test temperature
echo $VAL > $PORT

sleep 10

# Get Value at the WaziGate (with token)
TOKEN=`curl -s -X POST "http://wazigate.local/auth/token" -H  "accept: application/json" -H  "Content-Type: application/json" -d "{\"username\":\"admin\",\"password\":\"loragateway\"}"`
GATEWAY_VAL=`curl -s -X GET "http://wazigate.local/devices/602d0590784524000668d8cf" -H  "accept: application/json" -H "Cookie: Token=$TOKEN" | jq ".sensors[0].value"`

echo "Value from WaziGate: $VAL"

# Kill all child processes
trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM EXIT

if [ "$GATEWAY_VAL" = "$VAL" ]; then
    echo "Test success"
    exit 0
else
    echo "Test fail"
    exit 1
fi
