const mqtt = require('mqtt');

const MQTTPORT = 1883;
const MQTTIP = '13.124.176.173';
// const MQTTIP = 'localhost';
const MQTTQoS = 0;

var mqttCon;

function connect(callback) {
  mqttCon = mqtt.connect('http://' + MQTTIP + ':' + MQTTPORT);
  callback(mqttCon); 
}
module.exports.connect = connect;


function subscribe(topic) {
  var rescode;
  mqttCon.subscribe(topic, MQTTQoS, function (err, granted) {
    if(err){
      console.log('\x1b[31m%s\x1b[0m', "## Subscribe Err : " + err.message);
    }
  });
}
module.exports.subscribe = subscribe;


function unSubscribe(topic) {
  mqttCon.unsubscribe(topic, function (err) {
    if(err){
      console.log('\x1b[31m%s\x1b[0m', "## UnSubscriber Err : " + err.message);
    }
    //console.log('\x1b[32m%s\x1b[0m', "## UnSubscriber Topic : " + topic);
  });
}
module.exports.unSubscribe = unSubscribe;


function publish(topic, msg) {
  mqttCon.publish(topic, msg, MQTTQoS, function (err) {
    if(err){
      console.log('\x1b[31m%s\x1b[0m', "## Publish Err : " + err.message);
    }
  });
}
module.exports.publish = publish;
