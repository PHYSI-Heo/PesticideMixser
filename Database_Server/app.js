const express = require('express');
const bodyParser = require('body-parser');

var app = express();

app.set('port', process.env.PORT || 3000);
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
  extended: false
}));


app.listen(app.get('port'), function() {
  console.log("# Start Server..");
});


const manager = require('./setupManager');