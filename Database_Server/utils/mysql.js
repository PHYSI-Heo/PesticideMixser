const mysql = require('mysql');

const MYSQLIP = 'localhost';
const MYSQLID = 'root';
const MYSQLPWD = '1234';
const DBNAME = 'pmixer';

var dbInfo = {
  host: MYSQLIP,
  port: 3306,
  user: MYSQLID,
  password: MYSQLPWD,
  database : DBNAME,
  connectionLimit: 100,
  waitForConnections: true
};

module.exports.defaultInfo = {
  motor : '15',
  water : '30',
  pump : '30',
  agitator : '120',
};

const setupTable = "CREATE TABLE setting ( " +
  "did varchar(20) not null primary key, " +
  "powder_motor varchar(20) not null default '15', " +
  "powder_water varchar(20) not null default '30', " +
  "pesticide_motor varchar(20) not null default '15', " +
  "pesticide_water varchar(20) not null default '30', " +
  "air_pump varchar(20) not null default '15', " +
  "agitator varchar(20) not null default '120');";

var dbPool;

module.exports.createPool = () => {
  dbPool = mysql.createPool(dbInfo);
  if (dbPool) {
    console.log("# Create MySQL ThreadPool..Successful");
    query(setupTable, [], (res)=>{
      console.log("# Init Setup Table : " + res.resCode);
    });
  } else {
    console.log("# Create MySQL ThreadPool..Failed");
  }
}


function query (sql, params, callback) {
  var result = {};
  dbPool.getConnection((con_Err, con) => {
    if (con_Err) {
      // DB Connect Err
      result.resCode = 1002;
      console.log('\x1b[35m%s\x1b[0m', "## DB Connect Err : " + con_Err.message);
      callback(result);
    } else {
      con.query(sql, params, (query_Err, rows) => {
        if (query_Err) {
          // Query Error
          result.resCode = 1003;
          console.log('\x1b[35m%s\x1b[0m', "## DB Query Err : " + query_Err.message);
        } else {
          // Query Result
          //console.log('\x1b[36m%s\x1b[0m', "** DB Query Result ");
          console.log(rows);
          result.resCode = 1001;
          result.rows = rows;
        }
        con.release();
        callback(result);
      });
    }
  });
}
module.exports.query = query;

module.exports.initInfo = (did, callback) => {
  var sql = 'INSERT INTO setting(did) VALUES ( ? )';
  query(sql, [did], (res)=>{
      callback(res.resCode);
  });
};

module.exports.setInfo = (params, callback) => {
  var sql = 'UPDATE setting SET powder_motor = ?, powder_water = ?, pesticide_motor = ?, '
          + 'pesticide_water = ?, air_pump = ? , agitator = ? WHERE did = ?';
  query(sql, params, (res)=>{
      callback(res);
  });
};

module.exports.getInfo = (did, callback) => {
  var sql = 'SELECT * FROM setting WHERE did = ?';
  query(sql, [did], (res)=>{
      callback(res);
  });
};