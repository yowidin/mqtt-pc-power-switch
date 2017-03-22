var Accessory = require('../').Accessory;
var Service = require('../').Service;
var Characteristic = require('../').Characteristic;
var uuid = require('../').uuid;
var util = require('util');

var mqtt = require('mqtt');
var client = mqtt.connect('mqtt://localhost');

var BattleStation = {
   name: "Battle Station PC",
   pincode: "031-45-152",
   username: "FA:3C:EF:5A:1A:1A:",
   manufacturer: "yobasoft",
   model: "v1.1",
   serialNumber: "YOBA2",

   isOn: true,
   outputLogs: false,


   getOn: function() {
      if(this.outputLogs) console.log("'%s' on is %s", this.name, this.isOn);
      return this.isOn;
   },

   setOn: function(status) {
      if(this.outputLogs) console.log("'%s' turning the PC %s", this.name, status ? "on" : "off");
      this.isOn = status;

      client.publish("pc/switch", this.isOn ? 'on' : 'off');
   },

   identify: function() {
      if(this.outputLogs) console.log("Identify the '%s'", this.name);
   }
}

var sensorUUID = uuid.generate('yobasoft:accessories:battle-station-pc' + BattleStation.name);
var sensor = exports.accessory = new Accessory(BattleStation.name, sensorUUID);

sensor.username = BattleStation.username;
sensor.pincode = BattleStation.pincode;

sensor
  .getService(Service.AccessoryInformation)
       .setCharacteristic(Characteristic.Manufacturer, BattleStation.manufacturer)
       .setCharacteristic(Characteristic.Model, BattleStation.model)
       .setCharacteristic(Characteristic.SerialNumber, BattleStation.serialNumber);

// listen for the "identify" event for this Accessory
sensor.on('identify', function(paired, callback) {
  BattleStation.identify();
  callback();
});

sensor
  .addService(Service.Lightbulb, "PC")
  .getCharacteristic(Characteristic.On)
  .on('set', function(value, callback) {
     BattleStation.setOn(value);
     callback();
  })
  .on('get', function(callback) {
    callback(null, BattleStation.getOn());
  });


client.on('connect', function() {
   client.subscribe("pc/status")
});

client.on('message', function(topic, message) {
   if (topic =="pc/status") {
      BattleStation.isOn = message.toString() == 'on';
   }

   if (BattleStation.outputLogs) console.log("Got '%s' data: '%s'", topic, message);
});
