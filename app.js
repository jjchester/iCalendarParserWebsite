'use strict'

const mysql = require('mysql');
var connection;

// C library API
const ffi = require('ffi');
let sharedlib = ffi.Library('./libcal.dylib', {
  'getJSONCalendar': ['string', ['string']],
  'newCalendarFromJSON': ['string', ['string', 'string']],
  'addEventJSON':['string', ['string', 'string']],
});

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');

app.use(fileUpload());

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    console.log("No files were uploaded");
    return res.status(400).send('No files were uploaded.');
  }

  let uploadFile = req.files.uploadFile;

  // Use the mv() method to place the file somewhere on your server
  //console.log("Filename to upload: "+uploadFile.name);
  uploadFile.mv('uploads/' + uploadFile.name, function(error) {
    if(error) {
      return res.status(500).send(error);
    }
    let str = sharedlib.getJSONCalendar('./uploads/' + uploadFile.name);
    let cal = JSON.parse(str);
    //console.log(cal);
    if (typeof(cal.error) != 'undefined') {
      fs.unlink('./uploads/'+uploadFile.name,function(err){
        if(err) {
          console.log(err);
        }
        console.log("Error: "+cal.error);
        console.log("Successfully removed file.");
      });
      return res.status(500).send('Error: '+cal.error);
    }
    res.redirect('/');
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    console.log(err);
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      res.send('');
    }
  });
});

//******************** Your code goes here ******************** 

//A4 SQL FUNCTIONALITY

app.get('/sqlConnect', function(req, res) {
  var err = 0;
  let username = req.query.username;
  let password = req.query.password;
  let database = req.query.database;
  connection = mysql.createConnection({
   host     : 'dursley.socs.uoguelph.ca',
   user     :  username,
   password :  password,
   database :  database,
 });

  connection.query('SELECT 1 + 1 AS solution', (error, results, fields) => {
    if (error) {
      return res.status(500).send("Invalid username, DB name, or password");
    } else {
      let makeFileTable = 'CREATE TABLE IF NOT EXISTS FILE (cal_id INT AUTO_INCREMENT PRIMARY KEY,'
      +'file_name VARCHAR(60) NOT NULL, version INT NOT NULL,prod_id VARCHAR(256) NOT NULL);';
      connection.query(makeFileTable, function (err, rows, fields) {
        if (err){
          console.log("Something went wrong. "+err);
        } else {
          let makeEventTable = 'CREATE TABLE IF NOT EXISTS EVENT (event_id INT AUTO_INCREMENT PRIMARY KEY,'
          +'summary VARCHAR(1024), start_time DATETIME NOT NULL, location VARCHAR(60), organizer varchar(256),'
          +'cal_file INT NOT NULL, FOREIGN KEY(cal_file) REFERENCES FILE(cal_id) ON DELETE CASCADE)';
          connection.query(makeEventTable, function (err, rows, fields) {
            if (err){
              console.log("Something went wrong. "+err);
            } else {
              let makeAlarmTable = 'CREATE TABLE IF NOT EXISTS ALARM (alarm_id INT AUTO_INCREMENT PRIMARY KEY,'
              +'action VARCHAR(256) NOT NULL, `trigger` VARCHAR(256) NOT NULL, event INT NOT NULL);';
              connection.query(makeAlarmTable, function (err, rows, fields) {
                if (err){ 
                  console.log("Something went wrong. "+err);
                } else {
                  return res.sendStatus(200);
                }
              });
            }
          });
        }
      });
    }
  });
});

app.get('/clearTables', function(req, res) {
  //console.log("Called");
  connection.query('DELETE FROM ALARM;', function(err, rows, fields) {
    if (err) { 
      console.log("Something went wrong. "+err);
      return res.status(500).send("Something went wrong. "+err);
    } else {
      connection.query('ALTER TABLE ALARM AUTO_INCREMENT = 1', function(err, rows, fields) {
        if (err) {
          console.log("Something went wrong. "+err);
          return res.status(500).send("Something went wrong. "+err);
        } else {
          connection.query('DELETE FROM EVENT;', function(err, rows, fields) {
            if (err) {
              console.log("Something went wrong. "+err);
              return res.status(500).send("Something went wrong. "+err);
            } else {
             connection.query('ALTER TABLE EVENT AUTO_INCREMENT = 1', function(err, rows, fields) {
               if (err) {
                console.log("Something went wrong. "+err);
                return res.status(500).send("Something went wrong. "+err);
              } else {
                connection.query('DELETE FROM FILE;', function(err, rows, fields) {
                  if (err) { 
                    console.log("Something went wrong. "+err); 
                    return res.status(500).send("Something went wrong. "+err);
                  } else {
                    connection.query('ALTER TABLE FILE AUTO_INCREMENT = 1', function(err, rows, fields) {
                     if (err) {
                      console.log("Something went wrong. "+err);
                      return res.status(500).send("Something went wrong. "+err);
                    } else {
                      return res.sendStatus(200);
                    }
                  });
                  }
                });
              }
            });
           }
         });
        }
      });
    }
  });
});
app.get('/getAllEvents', function(req, res) {
  let query = 'SELECT * FROM EVENT ORDER BY start_time;';
  connection.query(query, function(err, rows, fields) {
    res.send(rows);
  });
});

app.get('/addCalToDB', function(req, res) {
  let cal = req.query;
  let filename = cal.filename;
  let queryStrFile = 'INSERT INTO FILE VALUES (null,\"'+filename+'\",\"'+cal.version+'\",\"'+cal.prodID+'\");';
  //console.log(queryStrFile);
  connection.query(queryStrFile, function(err, rows, fields) {
    if (err) {
      console.log("Something went wrong. "+err);
     // return res.status(500).send("Something went wrong. "+err);
   } else {
    let events = cal.events;
    for (var i = 0; i < events.length; i++) {
      var locationStr = "";
      var organizerStr = ""
      let properties = events[i].properties;
      if (properties != undefined) {
        for (var j = 0; j < properties.length; j++) {
          if (properties[j].propName === "LOCATION") {
            locationStr = properties[j].propDescr;
          }
          if (properties[j].propName === "ORGANIZER") {
            organizerStr = properties[j].propDescr;
          }
        }
      }
      let alarms = events[i].alarms;
      let dt = events[i].startDT.date + events[i].startDT.time;
        //console.log(dt);
        let dtStr = dt.substring(0,4) + '-' + dt.substring(4,6) + '-' + dt.substring(6,8)+'T' + dt.substring(8,10) + ':' + dt.substring(10,12)+':'+dt.substring(12,14);
        //console.log(dtStr);
        let summaryStr = events[i].summary;
        let calStr = '(SELECT cal_id FROM FILE WHERE file_name = \''+filename+'\')';
        let queryStrEvent = 'REPLACE INTO EVENT VALUES(null,\"'+events[i].summary+'\",\"'+dtStr+'\",\"'+locationStr+'\",\"'+organizerStr+'\",'+calStr+');';
        //console.log(queryStrEvent);
        connection.query(queryStrEvent, function(err, rows, fields) {
          //console.log("Alarm: "+JSON.stringify(rows));
          let insertID = rows.insertId;
          //console.log("Insert ID: "+insertID);
          if(err) {
            console.log("Something went wrong. "+err);
            //return res.status(500).send("Something went wrong. "+err);
          } else {
            if (alarms != undefined) {
              for (var j = 0; j < alarms.length; j++) {
                let evtStr = '(SELECT event_id FROM EVENT WHERE event_id = \''+calStr+'\')';
                let queryStrAlarm = 'REPLACE INTO ALARM VALUES(NULL,\"'+alarms[j].action+'\",\"' + alarms[j].trigger+'\",'+insertID+');';
                  //console.log(queryStrAlarm);
                  connection.query(queryStrAlarm, function(err, rows, fields) {
                    if (err) {
                      console.log("Something went wrong. "+err);
                    }
                  });
                }
              }
            }
          });
      }
    }
    //return sendStatus(200);
  });
  //console.log(queryStr);
  //console.log(cal);
  return res.sendStatus(200);
});

app.get('/getFileNames', function(req, res) {
  let query = 'SELECT file_name FROM FILE;';
  connection.query(query, function(err, rows, fields) {
    res.send(rows);
  });
});

app.get('/getEventsFromFile', function(req, res) {
  let filename = req.query.filename;
  var calID;
  console.log(filename);
  let query = 'SELECT * from FILE WHERE file_name=\"'+filename+'\";';
  connection.query(query, function(err, rows, fields) {
    if (err) {console.log("Something went wrong. "+err) }
      else {
        calID = rows[0].cal_id;

        let queryEvents = 'SELECT * from EVENT WHERE cal_file = '+calID+';';
        connection.query(queryEvents, function(err, rows, fields) {
          res.send(rows);
        });
      }
    //console.log(rows);
  });

});

app.get('/tableCount', function(req, res) {
  //console.log("Count");
  var val, val2, val3;
  let fileCount = 'SELECT COUNT(cal_id) FROM FILE;';
  connection.query(fileCount, function(err, rows, fields) {
    if (err) {console.log("Something went wrong. "+err)}
      else {
        let fieldName =fields[0].name;
        val = rows[0][fieldName];
        let eventCount = 'SELECT COUNT(event_id) FROM EVENT;';
        connection.query(eventCount, function(err, rows, fields) {
          if (err) { console.log("Something went wrong. "+err)}
            else {
             let fieldName2 =fields[0].name;
             val2 = rows[0][fieldName2];
             let alarmCount = 'SELECT COUNT(alarm_id) FROM ALARM;';
             connection.query(alarmCount, function(err, rows, fields) {
              if (err) {console.log("Something went wrong. "+err)}
                else {
                  let fieldName3 =fields[0].name;
                  val3 = rows[0][fieldName3];
                  let returnStr = 'Database has '+val+' files, '+val2+' events, and '+val3+' alarms.';
                  res.send({
                    count: returnStr,
                  });
                }
              });
           }
         });
      }
    });
});

/*******************************************************/
/*A3*/
app.get('/uploads', function(req , res){
  fs.readdir("./uploads", function(err, files) {
    //console.log(files);
    if (err === null) {
      res.send(files);
    } else {
      res.send(' ');
    }
  });
});

app.get('/deleteFile', function(req, res) {
  //console.log('Call');
  //console.log(req.query);
  //console.log('Filename: '+ req.query.filename);
  var error = false;
  let filename = req.query.filename;
  var str;
  var toSend;
  fs.unlink('./uploads/'+filename,function(err){
    if(err) {
      console.log("Error: " + err)
    } else {
      console.log("Successfully removed file.");
    }
  });
  res.send ({
    success: "Success"
  });
});

app.get('/createCalendarFromForm', function(req, res) {
  //console.log("Called");
  let cal = {
    version: req.query.version,
    prodID: req.query.prodID,
  };
  let stringcal = JSON.stringify(cal);
  //console.log(stringcal);
  let filename = req.query.filename;
  let addcal = sharedlib.newCalendarFromJSON(filename, stringcal);
  //console.log(addcal);

  var oldPath = 'uploads/tempcal.ics'
  var newPath = 'uploads/'+filename;

  fs.rename(oldPath, newPath, function (err) {
    if (err) throw err
  });
});

app.get('/createCale', function(req, res) {
  //console.log('Filename createcal: '+ req.query.filename);
  let str = sharedlib.getJSONCalendar('./uploads/' + req.query.filename);
  //console.log("Filename: "+req.query.filename);
  let cal = JSON.parse(str);
  if (typeof(cal.error) != 'undefined') {
    //console.log('Filename: '+req.query.filename +' error: '+ cal.error);
    cal = {
      error: cal.error
    };
  }
  res.send(cal);
});

app.get('/addEvent', function(req, res) {
  //console.log("Received "+JSON.stringify(req.query));
  let filename = req.query.filename;
  let event = req.query.event;
  let eventstring = JSON.stringify(event);
  //console.log("Event: "+eventstring);
  let name = "uploads/"+filename;
  let addevent = sharedlib.addEventJSON(name, eventstring);

  var oldPath = 'uploads/tempcal.ics'
  var newPath = 'uploads/'+filename;

  fs.rename(oldPath, newPath, function (err) {
    if (err) throw err
  });
  //console.log(addevent);
});

app.get('/getfile', function(req , res){

  res.send('{"result": true, "count":42}');
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);