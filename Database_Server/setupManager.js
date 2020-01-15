const db = require('./utils/mysql');
const mqtt = require('./utils/mqtt');

const TOPIC_UPDATE_INFO = "PM/SETUP/UPDATE";
const TOPIC_REQ_INFO = "PM/SETUP/REQ";

const TOPIC_SEND_INFO = "PM/SETUP/";


db.createPool();

mqtt.connect((conn)=>{
	if(conn){
		conn.on('connect', function () {
		  console.log('>> Connect MQTT Broker(Mosquitt)..');
		  mqtt.subscribe(TOPIC_UPDATE_INFO);
		  mqtt.subscribe(TOPIC_REQ_INFO);
		});
		// Mqtt Subscribe Event
		conn.on('message', function (topic, message) {
		  console.log(topic + " >> " + message);
		  if(topic == TOPIC_REQ_INFO){
		  	sendDeviceInfo(message);
		  }else if(topic == TOPIC_UPDATE_INFO){
		  	updateDeviceInfo(message);
		  }
		});
	}else{
		console.log("# MQTT Connected failed.");
	}
});


function sendDeviceInfo(did) {
	db.getInfo(did, (res)=>{		
		if(res.resCode == 1001){
			var resData;
			if(res.rows.length != 0){
				resData = res.rows[0].powder_motor + "," + res.rows[0].powder_water + "," + res.rows[0].pesticide_motor + "," 
					+ res.rows[0].pesticide_water + "," + res.rows[0].air_pump  + "," + res.rows[0].agitator;
				mqtt.publish(TOPIC_SEND_INFO + did, resData);
			}else{
				db.initInfo(did, (code)=>{
					resData = code;
					if(resData == "1001"){
						resData = db.defaultInfo.motor + "," + db.defaultInfo.water + "," + db.defaultInfo.motor + "," 
						+ db.defaultInfo.water + "," + db.defaultInfo.pump + "," + db.defaultInfo.agitator;
					}
					mqtt.publish(TOPIC_SEND_INFO + did, resData);
				});
			}			
		}else{
			mqtt.publish(TOPIC_SEND_INFO + did, res.resCode);
		}		
	});
}


function updateDeviceInfo(data) {
	var infos = data.toString().split(',');
	db.setInfo([infos[0], infos[1], infos[2], infos[3], infos[4], infos[5], infos[6]], (res)=>{
		var resData = "";
		if(res.resCode == 1001){
			resData = data;
		}
		mqtt.publish(TOPIC_SEND_INFO + infos[6], resData);
	});
}