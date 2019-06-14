// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    console.log("Page loaded");
    initializeModal();

});

$(window).on("load resize ", function() {
  var scrollWidth = $('.tbl-content').width() - $('.tbl-content table').width();
  $('.tbl-header').css({'padding-right':scrollWidth});
  populateFileTable();
  refreshFileList();
}).resize();

//A4----------------------------------------------
$('#login-form').submit(function(e) {
    e.preventDefault();
    let username = document.getElementById("usernameBox").value;
    let password = document.getElementById("passwordBox").value;
    let database = document.getElementById("dbBox").value;
    let login = {
        username: username,
        password: password,
        database: database,
    }
    $.ajax({
        type: 'get',
        url: 'sqlConnect',
        data: login,
        dataType: 'json',
        error: function(error) {
            if (error.responseText === 'OK') {
             var x = document.getElementById("loginPage");
             var y = document.getElementById("dbPanel");
             x.style.display = "none";
             y.style.display = "block";
         } else {
             var status = document.getElementById('login-status-panel');
             status.innerHTML = error.responseText;
             status.style.color = "red";
             console.log(error.responseText);
             document.getElementById("usernameBox").value = 'Username';
             document.getElementById("passwordBox").value = 'Password';
             document.getElementById("dbBox").value = 'Database name';
         }
     },
 });
});

$('#clearAllData').click(function(e){
    //console.log("cleared DB");
    $.ajax({
        type: 'get',
        url: '/clearTables',
        success: function(data) {
            var dbStatus = document.getElementById('db-status-panel');
            dbStatus.innerHTML = "Successfully cleared all tables.";
            dbStatus.style.color = "green";
        },
        error: function(error) {
            console.log("Success: "+error.responseText);
            if (error.responseText === 'OK') {

            } else {
                var dbStatus = document.getElementById('db-status-panel');
                dbStatus.innerHTML = error.responseText;
                dbStatus.style.color = "red";
            }
        }
    });
});

$('#storeAllFiles').click(function(e) {
    $('#clearAllData').click();
    setTimeout(function() {
    //console.log("Uploaded all files");
    //document.getElementById('db-status-panel').innerHTML = "uploaded files";
    var calendarArray = [];
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/uploads',
        success: function (data) {
            //We write the object to the console to show that the request was successful
            //console.log(data);
            for (file of data) {
                //Only add calendar files to the filename list
                if (file.split('.').pop() === "ics") {
                    //console.log(file);
                    //console.log(file);
                    $.ajax({
                        type: 'get',
                        dataType: 'json',
                        url: '/createCale',
                        data: {filename: file},
                        success: function(cal) {
                            cal.filename = cal.filename.substring(cal.filename.lastIndexOf("/")+1, cal.filename.length);
                            if (cal.filename === 'megaCal1.ics') {
                                console.log(cal);
                            }
                            let calendar = cal;
                                //console.log(cal);
                                $.ajax({
                                    type: 'get',
                                    url: 'addCalToDB',
                                    dataType: 'json',
                                    data: calendar,
                                    success: function(data) {
                                        console.log('success');
                                    },
                                    error: function(error) {
                                        console.log(error.responseText);
                                    }
                                });
                                var dbStatus = document.getElementById('db-status-panel');
                                dbStatus.innerHTML = "Successfully uploaded all files.";
                                dbStatus.style.color = "green";
                            },
                            error: function(error) {
                                console.log(error.responseText);
                            }
                        });
                }
            }
        },
    });
},500);
});

$('#displayDBstatus').click(function(e) {
    e.preventDefault();
    $.ajax({
        type: 'get',
        url: '/tableCount',
        dataType: 'json',
        success: function(data) {
            document.getElementById('db-status-panel').innerHTML = data.count;
            //console.log(data);
        },
        error: function(error) {
            console.log(error);
        },
    });
});

$('#displayEvents').click(function(e) {
    e.preventDefault();
    $.ajax({
        type: 'get',
        url: '/getAllEvents',
        dataType: 'json',
        success: function(data) {
            let str = ""
            for (event of data) {
                //console.log(event);
                str = str + 'Event '+data.indexOf(event) + '\nSummary: '+event.summary+'\nOrganizer: '+event.organizer+'\nStart date: '+event.start_time
                +'\nLocation: '+event.location+'\n\n';
            }
            console.log(str);
            $('#dbEventPanel').text(str);
            let panel = document.getElementById('dbEventPanel');
            panel.style.color = "black";
            let x = document.getElementById('closeDBPanelButton');
            x.style.display = "block";
        },
        error: function(error) {
            console.log(error);
        },
    })
});
$('#displayEventsForFile').click(function(e) {
    e.preventDefault();
    var hide = document.getElementById('dbPanel');
    hide.style.display = "none";
    var panel = document.getElementById('viewFileEventsPanel');
    viewFileEventsPanel.style.display = "block";
    $.ajax({
        type: 'get',
        url: '/getFileNames',
        dataType: 'json',
        success: function(data) {
            let select = document.getElementById('filenameDB');
            for (file of data) {
                //console.log(data);
                var opt = document.createElement("option");
                opt.text = file.file_name;
                select.add(opt);
            }
        },
        error: function(error) {
            console.log("F");
        },
    });
});

$('#filenameDB').change(function(e) {
    let filename = this.options[this.selectedIndex].text;
    $.ajax({
        type: 'get',
        url: '/getEventsFromFile',
        dataType: 'json',
        data: {
            filename: filename,
        },
        success: function(data) {
            var str = "Events: \n\n"
            for (event of data) {
                str = str + "Event: "+data.indexOf(event) + "\nStart date: "+event.start_time+"\nSummary: "+event.summary+'\n\n';
            }
             $('#dbEventPanel').text(str);
            let panel = document.getElementById('dbEventPanel');
            panel.style.color = "black";
            let x = document.getElementById('closeDBPanelButton');
            x.style.display = "block";
        },
    })
});

$('#closeFileSelect').click(function(e){
e.preventDefault();
    var hide = document.getElementById('viewFileEventsPanel');
    hide.style.display = "none";
    var panel = document.getElementById('dbPanel');
    panel.style.display = "block";
});

$('#displayConflictingEvents').click(function(e) {
    e.preventDefault();
    var events = [];
    var conflicts = [];
    $.ajax({
        type: 'get',
        url: '/getAllEvents',
        dataType: 'json',
        success: function(data) {
            var str = "CONFLICTS:\n";
            for (event of data) {
                for (var i = 0; i < events.length; i++) {
                    if (events[i].start_time === event.start_time) {
                        str = str + "Event summary: "+event.summary + "\nStart time: "+event.start_time+"\nOrganizer: "+event.organizer+"\n\n";
                        break;
                    }
                }
                events.push(event);
            }
            $('#dbEventPanel').text(str);
            let panel = document.getElementById('dbEventPanel');
            panel.style.color = "red";
            let x = document.getElementById('closeDBPanelButton');
            x.style.display = "block";
        },
        error: function(error) {
            console.log(error);
        },
    })
});
$('#closeDBPanelButton').click(function(e) {
    $('#dbEventPanel').text(" ");
    this.style.display = "none";
});
//A3----------------------------------------------
function showCalendarInputForm() {
    var x = document.getElementById("calendarInputForm");
    var y = document.getElementById("createButtons");
    if (x.style.display === "none") {
        x.style.display = "block";
        y.style.display = "none";
    } else {
        x.style.display = "none";
        y.style.display = "block";
    }
}



function populateFileTable() {
    var numFiles = 0;
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/uploads',
        success: function (data) {
            //We write the object to the console to show that the request was successful
            //console.log(data);
            for (file of data) {
                //Ensure that it only displays ics files in the upload directory
                filename = file;
                console.log(filename);
                if (file.split('.').pop() === "ics") {
                    createCalendarTable(filename);
                    numFiles++;
                }
            }
            if (numFiles === 0) {
                let table = document.getElementById('fileTable');
                let newRow = table.insertRow(-1);
                let cell1 = newRow.insertCell(0);
                let cell2 =  newRow.insertCell(1);
                let noFilesCell = newRow.insertCell(2);
                let cell3 = newRow.insertCell(3);
                let cell4 = newRow.insertCell(4);
                noFilesCell.append(document.createTextNode('No files'));
            }
        },
        fail: function(error) {
            // Non-200 return, do something with error
            console.log(error); 
        }
    });
}


function createCalendarTable(filename) {
    $('#fileTable tr>td').remove();
    let name = filename;
    let table = document.getElementById('fileTable');
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/createCale',
        data: {filename: name},
        success: function(cal) {
            let newRow = table.insertRow(-1);
            let filenameCell = newRow.insertCell(0);
            let versionCell = newRow.insertCell(1);
            let prodIDCell = newRow.insertCell(2);
            let eventsCell = newRow.insertCell(3);
            let propsCell = newRow.insertCell(4);
            if (cal.prodID ===  undefined) {
                filenameCell.append(document.createTextNode(filename + ' - (Remove)'));
                prodIDCell.append(document.createTextNode('Invalid file.'));
                filenameCell.style.color = "red";
                prodIDCell.style.color = "red";
                newRow.onclick = function() {
                    let conf = confirm("Would you like to remove this file?");
                    if (conf === true) {
                        deleteFile(name);
                        setTimeout(function() {
                            refreshFileList();
                            populateFileTable();
                        }, 1000);
                    }
                }
                return;
            } else {
                filenameCell.innerHTML =  '<a href="./uploads/'+filename+'">' + filename +'</a>';
                filenameCell.onclick = function() {
                    $("#file-select").val(filename).change();
                }
                versionCell.append(document.createTextNode(cal.version));
                prodIDCell.append(document.createTextNode(cal.prodID));
                eventsCell.append(document.createTextNode(cal.numEvents));
                propsCell.append(document.createTextNode(cal.numProps));

            }
        },
        error: function(error) {
            console.log('Error: ' + error.responseText); 
        }
    });
}

function deleteFile(filename) {
    name = filename;
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/deleteFile',
        data: {filename: name},
        success: function(data) {
            var x = document.getElementById("statusPanel");
            x.innerHTML = "File was successfully deleted.";
            x.style.color = "green";
        },
        error: function(error) {
            var x = document.getElementById("statusPanel");
            x.innerHTML = ("Error: " + error);
            x.style.color = "red";
        }
    });
}

function populateEventTable(filename) {
    let name = filename;
    let table = document.getElementById('eventTable');
    var panelStr;
    $('#eventTable tbody').empty();
    //$('#eventTable tr>td').remove();
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/createCale',
        //async: false,
        data: {filename: name},
        success: function(cal) {
            if (cal.error) {
                let newRow = table.insertRow(-1);
                let eventCell = newRow.insertCell(0);
                let dateCell = newRow.insertCell(1);
                let timeCell = newRow.insertCell(2);
                let summaryCell = newRow.insertCell(3);
                let propsCell = newRow.insertCell(4);
                let alarmsCell = newRow.insertCell(5);
                summaryCell.append(document.createTextNode("Invalid file"));
                return;
            }
            if (cal.events.length < 1) {
                let newRow = table.insertRow(-1);
                let eventCell = newRow.insertCell(0);
                let dateCell = newRow.insertCell(1);
                let timeCell = newRow.insertCell(2);
                let summaryCell = newRow.insertCell(3);
                let propsCell = newRow.insertCell(4);
                let alarmsCell = newRow.insertCell(5);
                summaryCell.append(document.createTextNode("No events."));
            }
            for(event of cal.events) {
                let newRow = table.insertRow(-1);
                let eventCell = newRow.insertCell(0);
                let dateCell = newRow.insertCell(1);
                let timeCell = newRow.insertCell(2);
                let summaryCell = newRow.insertCell(3);
                let propsCell = newRow.insertCell(4);
                let alarmsCell = newRow.insertCell(5);
                let alarms = event.alarms;
                let props = event.properties;
                alarmsCell.onclick = function() {
                    var modalheader = document.getElementById("modalHeader");
                    modalheader.innerHTML = "Alarms";
                    panelStr = 'Alarms: ';
                    var str = '';
                    if (alarms.length === 0) {
                        str = 'No alarms.';
                    } else {
                        for (alarm of alarms) {
                            str += 'Alarm ' + (alarms.indexOf(alarm) + 1) + '\n'  + 'Action: ' + alarm.action + '\n' + 'Trigger: ' + alarm.trigger + '\n\n';
                        }
                    }
                    var x = document.getElementById('alarmPropPanel');
                    showModalText(str);
                }
                propsCell.onclick = function() {
                    var modalheader = document.getElementById("modalHeader");
                    modalheader.innerHTML = "Properties";
                    panelStr = 'Properties: ';
                    var str = '';
                    if (props.length === 0) {
                        str = 'No additional properties';
                    } else {
                        for (prop of props) {
                            str += 'Property ' + (props.indexOf(prop) + 1) + '\n' + 'Name: '+ prop.propName + '\n' + 'Description: ' + prop.propDescr + '\n\n';
                        }
                    }
                    showModalText(str);
                }
                let eventNum = (cal.events.indexOf(event) + 1).toString();
                eventCell.append(document.createTextNode(eventNum));
                var date = (event.startDT.date.substring(0,4) + '/' + event.startDT.date.substring(4,6) + '/' + event.startDT.date.substring(6,8));
                dateCell.append(document.createTextNode(date));
                if (event.startDT.isUTC === true) {
                    var time = event.startDT.time.substring(0,2) + ':'+event.startDT.time.substring(2,4) +':'+event.startDT.time.substring(4,6) + '(UTC)';
                    timeCell.append(document.createTextNode(time));
                } else {
                    var time = event.startDT.time.substring(0,2) + ':'+event.startDT.time.substring(2,4) +':'+event.startDT.time.substring(4,6);
                    timeCell.append(document.createTextNode(time));
                }
                if (event.summary === '') {
                    summaryCell.append(document.createTextNode("No summary available."));
                } else {
                    summaryCell.append(document.createTextNode(event.summary));
                    summaryCell.onclick = function() {
                        var modalheader = document.getElementById("modalHeader");
                        modalheader.innerHTML = "Summary";
                        showModalText(this.innerHTML);
                    }
                }
                if (event.numProps == 0) {
                    propsCell.append(document.createTextNode('0'));
                } else {
                    propsCell.append(document.createTextNode(event.numProps));
                }
                if (event.numAlarms == 0) {
                    alarmsCell.append(document.createTextNode('0'));
                } else {
                    alarmsCell.append(document.createTextNode(event.numAlarms));
                }
            }
        },
        error: function(error) {
            console.log('Error: ' + error.responseText); 
        }
    });
}

function showModalText(to) {
    to = to.replace(/\n{2}/g, '&nbsp;</p><p>');
    to = to.replace(/\n/g, '&nbsp;<br />');
    to = '<p>' + to + '</p>';
    var modalBody = document.getElementById("modal-body");
    modalBody.innerHTML = to;
    var btn = document.getElementById("modalButton");
    btn.click();
}

function initializeModal() {
    // Get the modal
    var modal = document.getElementById('myModal');

    // Get the button that opens the modal
    var btn = document.getElementById("modalButton");

    // Get the <span> element that closes the modal
    var span = document.getElementsByClassName("close")[0];

    // When the user clicks the button, open the modal 
    btn.onclick = function() {
      modal.style.display = "block";
  }

    // When the user clicks on <span> (x), close the modal
    span.onclick = function() {
      modal.style.display = "none";
  }

    // When the user clicks anywhere outside of the modal, close it
    window.onclick = function(event) {
        if (event.target == modal) {
            modal.style.display = "none";
        }
    }
}

function showEventInputForm() {
    var x = document.getElementById("eventInputForm");
    var y = document.getElementById("createButtons");
    if (x.style.display === "none") {
        x.style.display = "block";
        y.style.display = "none";
    } else {
        x.style.display = "none";
        y.style.display = "block";
    }
}
$('#cancelCalendarForm').click(function(e){
    e.preventDefault();
    var x = document.getElementById("calendarInputForm");
    var y = document.getElementById("createButtons");
    if (x.style.display === "none") {
        x.style.display = "block";
        y.style.display = "none";
    } else {
        x.style.display = "none";
        y.style.display = "block";
    }
});

$('#cancelEventForm').click(function(e) {
    e.preventDefault();
    var x = document.getElementById("eventInputForm");
    var y = document.getElementById("createButtons");
    if (x.style.display === "none") {
        x.style.display = "block";
        y.style.display = "none";
    } else {
        x.style.display = "none";
        y.style.display = "block";
    }
});

$('#calendar-form').submit(function(e) {
    $("html, body").animate({ scrollTop: 0 }, "slow");
    var filename = document.getElementById('filenameBox').value;
    if (filename.split(".").pop() != "ics") {
        var x = document.getElementById("statusPanel");
        x.innerHTML = ("Error: invalid file extension.");
        x.style.color = "red";
        $("a[href='#top']").click(function() { $("html, body").animate({ scrollTop: 0 }, "slow"); return false; });
        return;
    }
    var prodID = document.getElementById('prodIDBox').value;
    var version = document.getElementById('versionBox').value;
    let toSend = {
        filename: filename,
        prodID: prodID,
        version: version,
    }
    $.ajax({
        url: '/createCalendarFromForm',
        type: 'get',
        dataType: 'json',
        data: toSend,
        success: function(data) {
            console.log("Success");
        },
        error: function(error) {
            console.log("Error: "+error.responseText);
        },
    });
    populateEventTable();
    populateFileTable();
    refreshFileList();
});

$('#event-form').submit(function(e) {
    var eventString;
    var UTCbool;
    var summaryString;
    var dateString;
    var UID;
    e.preventDefault();
    $("html, body").animate({ scrollTop: 0 }, "slow");
    var date = document.getElementById('startTimeBox').value;
    if (validateDT(date) == false) {
        var x = document.getElementById("statusPanel");
        x.innerHTML = "Invalid date";
        x.style.color = "red";
        return;
    }
    UID = document.getElementById('UIDbox').value;
    UTCbool = $("#UTCselect").children("option").filter(":selected").text();
    if (UTCbool === 'yes') {
        UTCbool = 'true';
    } else {
        UTCbool = 'false';
    }
    dateString = (date.substring(0,4) + date.substring(5,7) + date.substring(8,10) + 'T' + date.substring(11,13) + date.substring(14,16) + "00");
    if (UTCbool = 'true') {
        dateString+="Z";
    }
    var today = new Date();
    let year = today.getFullYear();
    let month = today.getMonth()+1;
    let day = today.getDate();
    //console.log(year);
    let hours = today.getHours();
    let mins = today.getMinutes();
    let seconds = today.getSeconds();
    if (day < 10) {
        day = "0"+day;
    }
    if (month < 10) {
        month = "0"+month;
    }
    if (hours < 10) {
        hours = "0"+hours;
    }
    if (mins < 10) {
        mins = "0"+mins;
    }
    if (seconds < 10) {
        seconds = "0"+seconds;
    }
    var dateTime = year +""+ month +""+ day +""+ "T" +""+ hours +""+ mins +""+ seconds + "Z";
    //console.log("DT:" +dateTime);
    //console.log("Date: "+dateTime);
    summaryString = document.getElementById('summaryBox').value;
    let event = {
        UID: UID,
        startDT: dateString,
        dtStamp: dateTime,
        summary: summaryString,
    }
    let sel = document.getElementById('event-select');
    let filename = sel.options[sel.selectedIndex].text;
    let toSend = {
        filename: filename, 
        event: event,
    }
    $.ajax({
        url: '/addEvent',
        dataType: 'json',
        data: toSend,
        type: 'get',
        success: function(data) {
            console.log("success");
        },
        error: function(error) {
            console.log("Error: "+error.responseText);
        }
    });
    //console.log(toSend);
    $('#cancelEventForm').click();
    populateEventTable();
    populateFileTable();
    refreshFileList();
    var x = document.getElementById("statusPanel");
    x.innerHTML = "Event was added successfully.";
    x.style.color = "green";

});

function validateDT(date) {
    if (date.length != 16) {
        return false;
    }
    return true;
}

$('#upload-form').submit(function(e) {
    $("html, body").animate({ scrollTop: 0 }, "slow");
    e.preventDefault();
    if (!(this[0].files[0])) {
        var x = document.getElementById("statusPanel");
        x.innerHTML = ("Error: No file selected for upload.");
        x.style.color = "red";
        return;
    }
    if (this[0].files[0].name.split('.').pop() != "ics") {
        var x = document.getElementById("statusPanel");
        x.innerHTML = ("Error: invalid file extension.");
        x.style.color = "red";
        return;
    }
    var formData = new FormData();
    formData.append('uploadFile', this[0].files[0]);
    $.ajax({
        url: '/upload',
        data: formData,
        processData: false,
        contentType: false,
        type: 'POST',
        success: function(data) {
            console.log('Successfully uploaded file');
            var x = document.getElementById("statusPanel");
            x.innerHTML = "File was successfully uploaded.";
            x.style.color = "green";
            return;
        },
        error: function(error) {
            console.log("Error: "+error.responseText);
            var x = document.getElementById("statusPanel");
            x.innerHTML = error.responseText + '. File was not uploaded.';
            x.style.color = "red";
        }
    });
            refreshFileList();
            $('#upload-form')[0].reset();
            populateFileTable();
});

function refreshFileList() {
    console.log('Clearing list...');
    var dropdown = document.getElementById('file-select');
    var length = dropdown.options.length;
    dropdown.options.length = 1;
    console.log('Updating list...');
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/uploads',
        success: function (data) {
            //We write the object to the console to show that the request was successful
            //console.log(data);
            var select = document.getElementById("file-select");
            var select2 = document.getElementById("event-select");
            for (file of data) {
                //Ensure that it only displays ics files in the upload directory
                filename = file;
                if (file.split('.').pop() === "ics") {
                    //console.log(file);
                    var opt = document.createElement("option");
                    var opt2 = document.createElement("option");
                    opt.text = file;
                    opt2.text = file;
                    select.add(opt);
                    select2.add(opt2);
                }
            }
        },
        fail: function(error) {
            // Non-200 return, do something with error
            console.log(error); 
        }
    });
}

$('#file-select').change(function(e) {
    let filename = this.options[this.selectedIndex].text;
    populateEventTable(filename);
    //refreshFileList();
});

