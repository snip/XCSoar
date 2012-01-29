String.prototype.lpad = function(padString, length) {
  var str = this;
  while (str.length < length)
  str = padString + str;
  return str;
}

function setupServer(client) {
  var start_timestamp = new Date().getTime();
  var pressure = 101325;
  setInterval(function() {
    var elapsed = (new Date().getTime() - start_timestamp) / 1000;
    var vario = Math.sin(elapsed / 3) * Math.cos(elapsed / 10) *
    Math.cos(elapsed / 20 + 2) * 3;

    var pressure_vario = vario * 12.5;
    var delta_pressure = pressure_vario * 48 / 1000;
    pressure += delta_pressure;

    var sentence = '_PRS ';
    sentence += Math.round(pressure).toString(16).toUpperCase().lpad('0', 8);

    console.log(sentence);
    client.write(sentence + '\n');
  }, 48);

  var battery_level = 11;
  setInterval(function() {
    var sentence = '_BAT ';
    if (battery_level <= 10)
    sentence += battery_level.toString(16).toUpperCase();
    else
    sentence += "*";

    console.log(sentence);
    client.write(sentence + '\n');

    if (battery_level == 0)
    battery_level = 11;
    else
    battery_level--;
  }, 11000);
}

var net = require('net');

var client = net.connect(10110, function() {
  console.log('connected');
  setupServer(client);
});
