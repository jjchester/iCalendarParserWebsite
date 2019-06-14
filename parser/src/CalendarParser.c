/**
 Justin Chester
 1018089
 jchester@uoguelph.ca
 **/
#include <stdio.h>
#include <ctype.h>

#include "CalendarParser.h"
#include "LinkedListAPI.h"

const char* BEGIN_TAG = "BEGIN";
const char* END_TAG = "END";
const char* CALENDAR_TAG = "VCALENDAR";
const char* EVENT_TAG = "VEVENT";
const char* ALARM_TAG = "VALARM";
const char* PRODID_TAG = "PRODID";
const char* VERSION_TAG = "VERSION";
const char* DTSTAMP_TAG = "DTSTAMP";
const char* DTSTART_TAG = "DTSTART";
const char* UID_TAG = "UID";
const char* ACTION_TAG = "ACTION";
const char* TRIGGER_TAG = "TRIGGER";

#define BUFFER_SIZE 2000
// Forward declarations
DateTime makeDate(char* dtString);
char* readLine(FILE* file, char* nextLine[]);
void unfold(char* outputLine[], char nextLine[]);
Property* makeProperty(char* name, char* description);
bool isValidEndTag(char* tag);
bool isValidDateTime(DateTime dt);
bool isValidProperty(Property* p, char* validProperties[], int numProperties);
ICalErrorCode validateEvent(Event* e);
ICalErrorCode validateAlarm(Alarm* a);
ICalErrorCode createEvent(FILE *fp, Event** newEvent, char** nextLine);
ICalErrorCode createAlarm(FILE *fp, Alarm** newAlarm, char** nextLine);
ICalErrorCode isValidFileExtension(char* fileName);
ICalErrorCode writeCalendar(char* fileName, const Calendar* obj);
ICalErrorCode writeEvent(FILE *fp, Event* toWrite);
ICalErrorCode writeProperty(FILE* fp, Property* toWrite);
ICalErrorCode writeAlarm(FILE* fp, Alarm* toWrite);
char* substring(char* string, int from, int to);
char* getJSONCalendar(char fileName[]);
char* propertyToJSON(Property* prop);
char* propertyListToJSON(List* properties);
char* alarmToJSON(const Alarm* alarm);
char* alarmListToJSON(const List* alarmList);
char* errCodeToStr(ICalErrorCode errCode);
char* newCalendarFromJSON(char filename[], char calendar[]);

char* getJSONCalendar(char fileName[]) {
    Calendar *obj;
    ICalErrorCode errCode = createCalendar(fileName, &obj);
    if (errCode != OK) {
        char* code = errCodeToStr(errCode);
        return code;
    }
    char* cal = calendarToJSON(fileName, obj);
    deleteCalendar(obj);
    return cal;
}

char* addEventJSON(char filename[], char event[]) {
    Calendar* obj = NULL;
    ICalErrorCode calCode = createCalendar(filename, &obj);
    if (calCode != OK) {
        return errCodeToStr(calCode);
    }
    Event* e = JSONtoEvent(event);
    addEvent(obj, e);
    
    ICalErrorCode writeCode = writeCalendar(filename, obj);
    deleteCalendar(obj);
    return errCodeToStr(writeCode);
}

char* newCalendarFromJSON(char filename[], char calendar[]) {
    Calendar* cal = JSONtoCalendar(calendar);
    ICalErrorCode writeCode = writeCalendar(filename, cal);
    deleteCalendar(cal);
    return errCodeToStr(writeCode);
}

char* errCodeToStr(ICalErrorCode errCode) {
    char error[15];
    char* returnError = malloc(30);
    switch(errCode) {
        case OK:
            strcpy(returnError, "OK");
            return returnError;
        case INV_FILE:
            strcpy(error, "INV_FILE");
            break;
        case INV_CAL:
            strcpy(error, "INV_CAL");
            break;
        case INV_VER:
            strcpy(error, "INV_VER");
            break;
        case DUP_VER:
            strcpy(error, "DUP_VER");
            break;
        case INV_PRODID:
            strcpy(error, "INV_PRODID");
            break;
        case DUP_PRODID:
            strcpy(error, "DUP_PRODID");
            break;
        case INV_EVENT:
            strcpy(error, "INV_EVENT");
            break;
        case INV_DT:
            strcpy(error, "INV_DT");
            break;
        case INV_ALARM:
            strcpy(error, "INV_ALARM");
            break;
        case WRITE_ERROR:
            strcpy(error, "WRITE_ERROR");
            break;
        case OTHER_ERROR:
            strcpy(error, "OTHER_ERROR");
            break;
    }
    sprintf(returnError, "{\"error\":\"%s\"}", error);
    return returnError;
}


ICalErrorCode createCalendar(char fileName[], Calendar** obj) {
    if (fileName == NULL) {
        return INV_FILE;
    }
    ICalErrorCode fileCode = isValidFileExtension(fileName);
    if (fileCode != OK) {
        return fileCode;
    }
    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        return INV_FILE;
    }
    (*obj) = malloc(sizeof(Calendar));
    (*obj)->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    (*obj)->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
    (*obj)->version = 0;
    strcpy((*obj)->prodID, "");
    
    char* nextLine = malloc(BUFFER_SIZE);
    fgets(nextLine, BUFFER_SIZE, fp);
    while (!feof(fp)) {
        char* line = readLine(fp, &nextLine);
        if (line[0] == ';') {
            continue;
        }
        if (strcmp(line, "error") == 0) {
            free(line);
            free(nextLine);
            fclose(fp);
            deleteCalendar(*obj);
            return INV_CAL;
        }
        if (line[strlen(line)-1] != '\n' || line[strlen(line)-2] != '\r') {
            free(line);
            free(nextLine);
            fclose(fp);
            deleteCalendar(*obj);
            return INV_CAL;
        }
        // Do parsing
        char* key = strtok(line, ";:");
        char* val = strtok(NULL, "\r\n");
        
        //printf("%s:%s\r\n",key,val);
        if (val == NULL) {
            if (strcmp(key, VERSION_TAG) == 0) {
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return INV_VER;
            }
            if (strcmp(key, PRODID_TAG) == 0) {
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return INV_PRODID;
            } else {
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return INV_CAL;
            }
        }
        if (strcmp(key, BEGIN_TAG) == 0 && strcmp(val, CALENDAR_TAG) == 0) {
            free(line);
            continue;
        }
        else if (strcmp(key, END_TAG) == 0 && strcmp(val, CALENDAR_TAG) == 0) {
            if (strlen((*obj)->prodID) > 0 && (*obj)->version != 0) {
                //If calendar has both product ID and version, it is valid. Return
                free(line);
                free(nextLine);
                fclose(fp);
                return OK;
            } else {
                //Otherwise it is an invalid calendar
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return INV_CAL;
            }
        }
        else if (strcmp(key, PRODID_TAG) == 0) {
            if (val == NULL) {
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return INV_PRODID;
            }
            if (strlen((*obj)->prodID) == 0) {
                strncpy((*obj)->prodID, val, 1000);
                //insertBack((*obj)->properties, makeProperty(key, val));
            } else {
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return DUP_PRODID;
            }
        }
        else if (strcmp(key, VERSION_TAG) == 0) {
            if ((*obj)->version == 0) {
                if (isdigit(val[0])) {
                    (*obj)->version = (float)atof(val);
                } else {
                    free(line);
                    free(nextLine);
                    fclose(fp);
                    deleteCalendar(*obj);
                    return INV_VER;
                }
            } else {
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return DUP_VER;
            }
        } else if (strcmp(key, BEGIN_TAG) == 0 && strcmp(val, EVENT_TAG) == 0) {
            Event* newEvent = NULL;
            ICalErrorCode code = createEvent(fp, &newEvent, &nextLine);
            if (code != OK) {
                free(line);
                free(nextLine);
                fclose(fp);
                deleteCalendar(*obj);
                return code;
            }
            insertBack((*obj)->events, newEvent);
        } else if (strcmp(key, BEGIN_TAG) == 0 && strcmp(val, CALENDAR_TAG) != 0 && strcmp(val, EVENT_TAG) != 0 && isValidEndTag(val)) {
            free(line);
            free(nextLine);
            fclose(fp);
            deleteCalendar(*obj);
            return INV_CAL;
        }
        else {
            Property* p = makeProperty(key, val);
            insertBack((*obj)->properties, p);
        }
        // Now that we're done with the line, free it
        free(line);
    }
    free(nextLine);
    fclose(fp);
    deleteCalendar(*obj);
    // Validate properties to check for errors
    return INV_CAL;
}

/**
 Helper functions that I think should be inside of the parser
 **/
ICalErrorCode createEvent(FILE *fp, Event** newEvent, char** nextLine) {
    (*newEvent) = malloc(sizeof(Event));
    (*newEvent)->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    (*newEvent)->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);
    while (!feof(fp)) {
        char *line = readLine(fp, nextLine);
        if (line[0] == ';') {
            continue;
        }
        if (strcmp(line, "error") == 0) {
            return INV_EVENT;
        }
        //Do all error checks before doing property creation
        char *key = strtok(line, ";:");
        char *val = strtok(NULL, "\r\n");
        if (val == NULL) {
            return INV_EVENT;
        }
        //printf("%s:%s\r\n",key,val);
        if (strcmp(key, DTSTAMP_TAG) == 0 && strlen(val) > 0) {
            (*newEvent)->creationDateTime = makeDate(val);
            if (strcmp((*newEvent)->creationDateTime.date, "INVALID") == 0) {
                return INV_DT;
            }
        } else if (strcmp(key, UID_TAG) == 0 && strlen(val)>0) {
            strcpy((*newEvent)->UID, val);
        }else if (strcmp(key, END_TAG) == 0 && strcmp(val, EVENT_TAG) == 0) {
            if (isValidDateTime((*newEvent)->creationDateTime) && strlen((*newEvent)->UID) > 0) {
                free(line);
                return OK;
            } else {
                //If the event does not have a UID or dtstamp, it is an invalid event
                free(line);
                return INV_EVENT;
            }
        }
        else if (strcmp(key, DTSTART_TAG) == 0) {
            (*newEvent)->startDateTime = makeDate(val);
        }
        else if (strcmp(key, BEGIN_TAG) == 0 && strcmp(val, ALARM_TAG) == 0) {
            Alarm* newAlarm = NULL;
            ICalErrorCode code = createAlarm(fp, &newAlarm, nextLine);
            if (code != OK) {
                return code;
            }
            insertBack((*newEvent)->alarms, newAlarm);
        } else if (strcmp(key, END_TAG) == 0 && strcmp(val, EVENT_TAG) != 0 && isValidEndTag(val)) {
            //Invalid nesting of begin/ends, invalid calendar
            free(line);
            return INV_EVENT;
        }
        
        else {
            insertBack((*newEvent)->properties, makeProperty(key, val));
        }
        free(line);
    }
    return INV_EVENT;
}

ICalErrorCode createAlarm(FILE *fp, Alarm** newAlarm, char** nextLine) {
    (*newAlarm) = malloc(sizeof(Alarm));
    (*newAlarm)->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    (*newAlarm)->trigger = NULL;
    strcpy((*newAlarm)->action, "");
    while (!feof(fp)) {
        char *line = readLine((fp), nextLine);
        if (line[0] == ';') {
            continue;
        }
        if (strcmp(line, "error") == 0) {
            return INV_ALARM;
        }
        //Do all error checks before doing property creation
        char *key = strtok(line, ";:");
        char *val = strtok(NULL, "\r\n");
        if (val == NULL) {
            free(line);
            return INV_ALARM;
        }
        //printf("%s:%s\r\n",key,val);
        if (strcmp(key, TRIGGER_TAG) == 0) {
            (*newAlarm)->trigger = malloc(strlen(val)+1);
            strncpy((*newAlarm)->trigger, val, strlen(val)+1);
        }
        else if (strcmp(key, ACTION_TAG) == 0) {
            strncpy((*newAlarm)->action, val, strlen(val)+1);
        }
        else if (strcmp(key, END_TAG) == 0 && strcmp(val, ALARM_TAG) == 0) {
            //Alarm must have trigger and action, otherwise it is an invalid alarm
            if (strlen((*newAlarm)->action) > 0 && (*newAlarm)->trigger != NULL) {
                free(line);
                return OK;
            } else {
                free(line);
                return INV_ALARM;
            }
        }
        else if (strcmp(key, END_TAG) == 0 && strcmp(val, ALARM_TAG) != 0 && isValidEndTag(val)) {
            //Invalid nesting of begin/ends, invalid calendar
            free(line);
            return INV_ALARM;
        } else {
            insertBack((*newAlarm)->properties, makeProperty(key, val));
        }
        free(line);
    }
    return INV_ALARM;
}

char* readLine(FILE* file, char* nextLine[]) {
    char* outputLine = malloc(BUFFER_SIZE);
    if (*nextLine == NULL) {
        *nextLine = malloc(BUFFER_SIZE);
        fgets(*nextLine, BUFFER_SIZE, file);
    }
    while (!feof(file)) {
        // Copy input line into our output buffer
        strncpy(outputLine, *nextLine, BUFFER_SIZE);
        while (fgets(*nextLine, BUFFER_SIZE, file)) {
            // Skip blank lines
            if (*nextLine[0] == '\r' || *nextLine[0] == '\n') {
                continue;
            }
            // This is not a folded line, terminating condition
            if (*nextLine[0] != ' ' && *nextLine[0] != '\t') {
                break;
            }
            unfold(&outputLine, *nextLine);
        }
        return outputLine;
    }
    return NULL;
}

void unfold(char* outputLine[], char nextLine[]) {
    strtok(*outputLine, "\r\n");
    // This math all works because we shift the nul byte as well
    int nextLineLength = (int)strlen(nextLine);
    memmove(nextLine, nextLine + 1, nextLineLength);
    *outputLine = realloc(*outputLine, strlen(*outputLine) + nextLineLength);
    strncat(*outputLine, nextLine, strlen(*outputLine) + nextLineLength);
}

DateTime makeDate(char* dtString) {
    DateTime dt;
    // This chops off the attributes if they were shoved into the value. Generally just TZID.
    char* separator = strstr(dtString, ":");
    if (separator != NULL) {
        dtString = &separator[1];
    }
    if (strstr(dtString, "T") == NULL) {
        strcpy(dt.date, "INVALID");
        return dt;
    }
    dt.UTC = (strstr(dtString, "Z") == NULL) ? false : true;
    
    char* dateString = substring(dtString, 0, 7);
    int dLength = (int)strlen(dateString);
    for (int i = 0; i < dLength; i++) {
        if (!isdigit(dateString[i])) {
            strcpy(dt.date, "INVALID");
            return dt;
        }
    }
    char* timeString = substring(dtString, 9, 14);
    strncpy(dt.date, dateString, 9);
    strncpy(dt.time, timeString, 7);
    free(dateString);
    free(timeString);
    return dt;
}

Property* makeProperty(char* name, char* description) {
    // Allocate space for struct and the flexible array member string
    Property* prop = malloc(sizeof(Property) + strlen(description) + 1);
    strncpy(prop->propName, name, 200);
    strncpy(prop->propDescr, description, strlen(description)+1);
    return prop;
}

bool isValidEndTag(char* tag) {
    return (strcmp(tag, EVENT_TAG) == 0 || strcmp(tag, ALARM_TAG) == 0 || strcmp(tag, CALENDAR_TAG) == 0);
}

bool isValidDateTime(DateTime dt) {
    if (strlen(dt.date) == 8 && strlen(dt.time) == 6) {
        return true;
    }
    return false;
}

ICalErrorCode isValidFileExtension(char* fileName) {
    int length = (int)strlen(fileName);
    if (fileName[length-1] != 's' || fileName[length-2] != 'c' || fileName[length-3] != 'i') {
        return INV_FILE;
    }
    return OK;
}

/**
 Functions to write a calendar object to a file
 **/
ICalErrorCode writeCalendar(char* fileName, const Calendar* obj) {
    FILE *writeFile = fopen("uploads/tempcal.ics", "w+");
    if (writeFile == NULL) {
        return WRITE_ERROR;
    }
    if (fileName == NULL || strlen(fileName) == 0) {
        return WRITE_ERROR;
    }
    ICalErrorCode fileCode = isValidFileExtension(fileName);
    if (fileCode != OK) {
        return WRITE_ERROR;
    }
    fprintf(writeFile,"BEGIN:VCALENDAR\r\n");
    fprintf(writeFile, "VERSION:%.1lf\r\n", obj->version);
    fprintf(writeFile, "PRODID:%s\r\n", obj->prodID);
    Node* properties = obj->properties->head;
    while (properties != NULL) {
        writeProperty(writeFile, (Property*)properties->data);
        properties = properties->next;
    }
    Node* events = obj->events->head;
    while (events != NULL) {
        ICalErrorCode eventCode = writeEvent(writeFile, (Event*)events->data);
        if (eventCode != OK) {
            return WRITE_ERROR;
        }
        events = events->next;
    }
    fprintf(writeFile, "END:VCALENDAR\r\n");
    fclose(writeFile);
    return OK;
}

ICalErrorCode writeEvent(FILE* fp, Event* toWrite) {
    fprintf(fp, "BEGIN:VEVENT\r\n");
    fprintf(fp, "UID:%s\r\n", toWrite->UID);
    if (toWrite->creationDateTime.UTC == true) {
        fprintf(fp, "DTSTAMP:%sT%sZ\r\n", toWrite->creationDateTime.date, toWrite->creationDateTime.time);
    } else {
        fprintf(fp, "DTSTAMP:%sT%s\r\n", toWrite->creationDateTime.date, toWrite->creationDateTime.time);
    }
    if (strlen(toWrite->startDateTime.date) > 0) {
        if (toWrite->startDateTime.UTC == true) {
            fprintf(fp, "DTSTART:%sT%sZ\r\n", toWrite->startDateTime.date, toWrite->startDateTime.time);
        } else {
            fprintf(fp, "DTSTART:%sT%s\r\n", toWrite->startDateTime.date, toWrite->startDateTime.time);
        }
    }
    Node *properties = toWrite->properties->head;
    while (properties != NULL) {
        writeProperty(fp, (Property*)properties->data);
        properties = properties->next;
    }
    Node *alarms = toWrite->alarms->head;
    while (alarms != NULL) {
        writeAlarm(fp, (Alarm*)alarms->data);
        alarms = alarms->next;
    }
    
    fprintf(fp,"END:VEVENT\r\n");
    return OK;
}

ICalErrorCode writeAlarm(FILE* fp, Alarm* toWrite) {
    fprintf(fp, "BEGIN:VALARM\r\n");
    fprintf(fp, "TRIGGER;%s\r\n", toWrite->trigger);
    fprintf(fp, "ACTION:%s\r\n", toWrite->action);
    Node *properties = toWrite->properties->head;
    while (properties != NULL) {
        writeProperty(fp, (Property*)properties->data);
        properties = properties->next;
    }
    fprintf(fp, "END:VALARM\r\n");
    return OK;
}

ICalErrorCode writeProperty(FILE* fp, Property* toWrite) {
    //If the property has parameters
    if (strstr(toWrite->propDescr, ":") != NULL && strcmp(toWrite->propName, "ATTACH") != 0 && strcmp(toWrite->propName, "SUMMARY") != 0) {
        fprintf(fp, "%s;%s\r\n", toWrite->propName, toWrite->propDescr);
    } else {
        fprintf(fp, "%s:%s\r\n", toWrite->propName, toWrite->propDescr);
    }
    return OK;
}

/**
 Validation Functions for Calendar Object
 **/
ICalErrorCode validateCalendar(const Calendar* obj) {
    char *calendarProperties[4] = {"CALSCALE", "METHOD", "PRODID", "VERSION"};
    int numCalscale = 0;
    int numMethods = 0;
    int numProdIDs = 0;
    int numVersions = 0;
    if (obj == NULL) {
        return INV_CAL;
    }
    if (obj->events == NULL || obj->events->length == 0) {
        return INV_CAL;
    }
    if (obj->properties == NULL) {
        return INV_CAL;
    }
    if (obj->version == 2) {
        numVersions++;
    }
    if (strlen(obj->prodID) > 0) {
        numProdIDs++;
    }
    //Validating properties in the calendar property list
    Node *properties = obj->properties->head;
    while (properties != NULL) {
        Property* p = (Property*)properties->data;
        if (isValidProperty(p, calendarProperties, 4)) {
            if (strcmp(p->propName, "CALSCALE") == 0) {
                numCalscale++;
            }
            else if (strcmp(p->propName, "METHOD") == 0) {
                numMethods++;
            }
            else if (strcmp(p->propName, "PRODID") == 0) {
                numProdIDs++;
            }
            else if (strcmp(p->propName, "VERSION") == 0) {
                numVersions++;
            }
        } else {
            return INV_CAL;
        }
        properties = properties->next;
    }
    if (numCalscale > 1) {
        return INV_CAL;
    }
    if (numMethods > 1) {
        return INV_CAL;
    }
    if (numProdIDs != 1) {
        return INV_CAL;
    }
    if (numVersions != 1) {
        return INV_CAL;
    }
    Node* events = obj->events->head;
    while (events != NULL) {
        Event* e = (Event*)events->data;
        ICalErrorCode eventCode = validateEvent(e);
        if (eventCode != OK) {
            return eventCode;
        }
        events = events->next;
    }
    return OK;
}

ICalErrorCode validateEvent(Event* e) {
    char* eventProperties[] = {"ATTACH", "CATEGORIES", "CLASS", "COMMENT", "DESCRIPTION", "GEO", "LOCATION", "PRIORITY", "RESOURCES", "STATUS", "SUMMARY", "DATE-TIME", "DTEND", "DTSTART", "DURATION", "TRANSP", "ATTENDEE", "CONTACT", "ORGANIZER", "RECURRENCE-ID", "RELATED-TO", "URL", "UID", "EXDATE", "RDATE", "RRULE", "CREATED", "DTSTAMP", "LAST-MODIFIED", "SEQUENCE"
    };
    
    int numProperties = 30;
    int numTransp = 0; //Must be 0 or 1
    int numURL = 0; //Must be 0 or 1
    int numUID = 0; //Must be 1
    int numDTSTART = 0; //Must be 0 or 1
    int numDTSTAMP = 0; //Must be 1
    int numCreated = 0; //Must be 0 or 1
    int numStatus = 0; //Must be 0 or 1
    if (e->alarms == NULL) {
        return INV_EVENT;
    }
    if (e->properties == NULL) {
        return INV_EVENT;
    }
    if (!isValidDateTime(e->creationDateTime)) {
        return INV_EVENT;
    }
    if (strlen(e->UID) > 0) {
        numUID++;
    }
    if (strlen(e->creationDateTime.date) > 0) {
        numDTSTAMP++;
    }
    if (strlen(e->startDateTime.date) > 0) {
        numDTSTART++;
    }
    Node* properties = e->properties->head;
    while (properties != NULL) {
        Property* p = (Property*)properties->data;
        if (isValidProperty(p, eventProperties, numProperties)) {
            if (strcmp("TRANSP", p->propName) == 0) {
                numTransp++;
            }
            else if (strcmp("URL", p->propName) == 0) {
                numURL++;
            }
            else if (strcmp("UID", p->propName) == 0) {
                numUID++;
            }
            else if (strcmp("DTSTART", p->propName) == 0) {
                numDTSTART++;
            }
            else if (strcmp("CREATED", p->propName) == 0) {
                numCreated++;
            }
            else if (strcmp("DTSTAMP", p->propName) == 0) {
                numDTSTAMP++;
            }
            else if (strcmp("STATUS", p->propName) == 0) {
                numStatus++;
            }
        } else {
            return INV_EVENT;
        }
        properties = properties->next;
    }
    if (numTransp > 1) {
        return INV_EVENT;
    }
    if (numURL > 1) {
        return INV_EVENT;
    }
    if (numUID != 1) {
        return INV_EVENT;
    }
    if (numDTSTART > 1) {
        return INV_EVENT;
    }
    if (numCreated > 1) {
        return INV_EVENT;
    }
    if (numDTSTAMP != 1) {
        return INV_EVENT;
    }
    if (numStatus > 1) {
        return INV_EVENT;
    }
    Node* alarms = e->alarms->head;
    while (alarms != NULL) {
        Alarm* a = (Alarm*)alarms->data;
        ICalErrorCode alarmCode = validateAlarm(a);
        if (alarmCode != OK) {
            return alarmCode;
        }
        alarms = alarms->next;
    }
    return OK;
}

ICalErrorCode validateAlarm(Alarm* a) {
    char* alarmProperties[] = {"ATTACH", "DESCRIPTION", "SUMMARY", "DURATION", "ACTION", "REPEAT", "TRIGGER"};
    int numProperties = 7;
    int numDescription = 0; //Must be 0 or 1
    int numAction = 0;  //Must be 1
    int numTrigger = 0;  //Must be 1
    if (a->properties == NULL || a->properties->length == 0) {
        return INV_ALARM;
    }
    if (a->trigger == NULL || !(strlen(a->trigger) > 0) || !(strlen(a->action) > 0)) {
        return INV_ALARM;
    }
    if (strlen(a->action) > 0) {
        numAction++;
    }
    if (strlen(a->trigger) > 0) {
        numTrigger++;
    }
    Node* properties = a->properties->head;
    while (properties != NULL) {
        Property* p = (Property*)properties->data;
        if (isValidProperty(p, alarmProperties, numProperties)) {
            if (strcmp("DESCRIPTION", p->propName) == 0) {
                numDescription++;
            }
            else if (strcmp("ACTION", p->propName) == 0) {
                numAction++;
            }
            else if (strcmp("TRIGGER", p->propName) == 0) {
                numTrigger++;
            }
        } else {
            return INV_ALARM;
        }
        properties = properties->next;
    }
    if (numDescription > 1) {
        return INV_ALARM;
    }
    if (numAction != 1) {
        return INV_ALARM;
    }
    if (numTrigger != 1) {
        return INV_ALARM;
    }
    return OK;
}

bool isValidProperty(Property* p, char* validProperties[], int numProperties) {
    for (int i = 0; i < numProperties; i++) {
        if (strcmp(validProperties[i], p->propName) == 0) {
            if (strlen(p->propDescr) > 0) {
                return true;
            }
        }
    }
    return false;
}

char* dtToJSON(DateTime prop) {
    //{"date":"date val","time":"time val","isUTC":utcVal}
    char* buffer = malloc(BUFFER_SIZE);
    if (prop.UTC == true) {
        snprintf(buffer, BUFFER_SIZE, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":%s}", prop.date, prop.time, "true");
    } else {
        snprintf(buffer, BUFFER_SIZE, "{\"date\":\"%s\",\"time\":\"%s\",\"isUTC\":%s}", prop.date, prop.time, "false");
    }
    buffer = realloc(buffer, strlen(buffer)+1);
    return buffer;
}
char* alarmToJSON(const Alarm* alarm) {
    char* props = propertyListToJSON(alarm->properties);
    int length = 10*(int)(strlen(alarm->action) + strlen(alarm->trigger) + strlen(props) + 1);
    char* buffer = malloc(length);
    snprintf(buffer, length, "{\"action\":\"%s\",\"trigger\":\"%s\",\"properties\":%s}", alarm->action, alarm->trigger, props);
    buffer = realloc(buffer, strlen(buffer)+1);
    free(props);
    return buffer;
}
char* alarmListToJSON(const List* alarmList) {
    if (alarmList->length == 0) {
        char* returnString = malloc(3);
        strcpy(returnString, "[]");
        return returnString;
    }
    char* buffer = NULL;
    buffer = malloc(BUFFER_SIZE);
    strcpy(buffer, "[");
    char* alarm = NULL;
    Node* alarms = alarmList->head;
    while (alarms != NULL) {
        Alarm* a = (Alarm*)alarms->data;
        alarm = alarmToJSON(a);
        int length = (int)(strlen(buffer) + strlen(alarm) + 1);
        buffer = realloc(buffer, length + 1);
        strncat(buffer, alarm, length);
        free(alarm);
        if (alarms->next != NULL) {
            buffer = realloc(buffer, strlen(buffer) + 2);
            strncat(buffer, ",", 1);
        }
        alarms = alarms->next;
    }
    buffer = realloc(buffer, strlen(buffer) + 2);
    strcat(buffer, "]");
    if (buffer[0] == '[' && buffer[1] == ']') {
        printf("break");
    }
    return buffer;
}

char* propertyToJSON(Property* prop) {
    int length = 10*(int)(strlen(prop->propName) + strlen(prop->propDescr) + 1);
    char* buffer = malloc(length);
    snprintf(buffer, length, "{\"propName\":\"%s\",\"propDescr\":\"%s\"}", prop->propName, prop->propDescr);
    buffer = realloc(buffer, strlen(buffer)+1);
    return buffer;
}

char* propertyListToJSON(List* propertyList) {
    if (propertyList->length == 0) {
        char* returnString = malloc(3);
        strcpy(returnString, "[]");
        return returnString;
    }
    char* buffer = NULL;
    buffer = malloc(BUFFER_SIZE);
    strcpy(buffer, "[");
    char* prop = NULL;
    Node* props = propertyList->head;
    while (props != NULL) {
        Property* p = (Property*)props->data;
        prop = propertyToJSON(p);
        int length = (int)(strlen(buffer) + strlen(prop) + 1);
        buffer = realloc(buffer, length + 1);
        strncat(buffer, prop, length);
        free(prop);
        if (props->next != NULL) {
            buffer = realloc(buffer, strlen(buffer) + 2);
            strncat(buffer, ",", 1);
        }
        props = props->next;
    }
    buffer = realloc(buffer, strlen(buffer) + 2);
    strcat(buffer, "]");
    if (buffer[0] == '[' && buffer[1] == ']') {
        printf("break");
    }
    return buffer;
}

char* eventToJSON(const Event* event) {
    //{"startDT":DTval,"numProps":propVal,"numAlarms":almVal,"summary":"sumVal"}
    char* buffer = malloc(BUFFER_SIZE);
    char* sumVal = malloc(BUFFER_SIZE);
    if (event == NULL) {
        strncpy(buffer, "{}", 3);
        buffer = realloc(buffer, 3);
        return buffer;
    }
    strcpy(sumVal, "");
    char* dtVal = dtToJSON(event->startDateTime);
    int numProps = 3 + event->properties->length;
    int numAlarms = event->alarms->length;
    Node* summaryProp = event->properties->head;
    while (summaryProp != NULL) {
        Property* p = (Property*)summaryProp->data;
        if (strcmp(p->propName, "SUMMARY") == 0) {
            strncpy(sumVal, p->propDescr, strlen(p->propDescr)+1);
            sumVal = realloc(sumVal, strlen(sumVal)+1);
            break;
        }
        summaryProp = summaryProp->next;
    }
    char* alarms = alarmListToJSON(event->alarms);
    char* properties = propertyListToJSON(event->properties);
    int length = 5*BUFFER_SIZE;
    buffer = realloc(buffer, length);
    snprintf(buffer, length, "{\"UID\":\"%s\",\"startDT\":%s,\"properties\":%s,\"numProps\":%d,\"alarms\":%s,\"numAlarms\":%d,\"summary\":\"%s\"}",event->UID, dtVal, properties, numProps, alarms, numAlarms, sumVal);
    buffer = realloc(buffer, strlen(buffer)+1);
    free(dtVal);
    free(sumVal);
    free(properties);
    free(alarms);
    return buffer;
}

char* eventListToJSON(const List* eventList) {
    if (eventList->length == 0) {
        char* returnString = malloc(3);
        strcpy(returnString, "[]");
        return returnString;
    }
    char* buffer = NULL;
    buffer = malloc(BUFFER_SIZE);
    strcpy(buffer, "[");
    char* evt = NULL;
    Node* events = eventList->head;
    while (events != NULL) {
        Event* e = (Event*)events->data;
        evt = eventToJSON(e);
        int length = (int)(strlen(buffer) + strlen(evt) + 1);
        buffer = realloc(buffer, length + 1);
        strncat(buffer, evt, length);
        free(evt);
        if (events->next != NULL) {
            buffer = realloc(buffer, strlen(buffer) + 2);
            strncat(buffer, ",", 1);
        }
        events = events->next;
    }
    buffer = realloc(buffer, strlen(buffer) + 2);
    strcat(buffer, "]");
    return buffer;
}

char* calendarToJSON(const char* filename, const Calendar* cal) {
    //{"version":verVal,"prodID":"prodIDVal","numProps":propVal,"numEvents":evtVal}
    char* buffer = NULL;
    if (cal == NULL) {
        strncpy(buffer, "{}", 3);
        buffer = realloc(buffer, 3);
        return buffer;
    }
    char* properties = propertyListToJSON(cal->properties);
    char* events = eventListToJSON(cal->events);
    int length = 15*BUFFER_SIZE;
    buffer = malloc(length);
    snprintf(buffer, length, "{\"filename\":\"%s\",\"version\":%.0f,\"prodID\":\"%s\",\"properties\":%s,\"numProps\":%d,\"events\":%s,\"numEvents\":%d}", filename, cal->version, cal->prodID, properties, cal->properties->length + 2, events, cal->events->length);
    buffer = realloc(buffer, strlen(buffer)+1);
    free(properties);
    free(events);
    return buffer;
}

Calendar* JSONtoCalendar(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    Calendar* cal = malloc(sizeof(Calendar));
    cal->version = 0;
    char* calStr = malloc(strlen(str)+1);
    strcpy(calStr, str);
    cal->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    cal->events = initializeList(&printEvent, &deleteEvent, &compareEvents);
    char* tagStr = strtok(calStr, "\"");
    char* tempStr = malloc(BUFFER_SIZE);
    while (tagStr != NULL) {
        if (strcmp(tagStr, "version") == 0) {
            tagStr = strtok(NULL, "\"");
            tempStr = strtok(NULL, "\"");
            cal->version = atof(tempStr);
        }
        if (strcmp(tagStr, "prodID") == 0) {
            tagStr = strtok(NULL, "\"");
            tempStr = strtok(NULL, "\"");
            strcpy(cal->prodID, tempStr);
        }
        tagStr = strtok(NULL, "\"");
    }
    return cal;
}

Event* JSONtoEvent(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    char* evtString = malloc(strlen(str)+1);
    strcpy(evtString, str);
    Event* evt = malloc(sizeof(Event));
    //strncpy(evt->UID, "\0", 1);
    evt->properties = initializeList(&printProperty, &deleteProperty, &compareProperties);
    evt->alarms = initializeList(&printAlarm, &deleteAlarm, &compareAlarms);
    char* startDate = malloc(30);
    char* creationDate = malloc(30);
    char* summaryString = malloc(BUFFER_SIZE);
    char* tagStr = strtok(evtString, "\"");
    char* tempStr = malloc(BUFFER_SIZE);
    //{"UID":"UID1","startDT":"20190429T120000Z","dtStamp":"20190404T194542Z","summary":"sumarye"}
    while (tagStr != NULL) {
        if (strcmp(tagStr, "UID") == 0) {
            tagStr = strtok(NULL, "\"");
            tempStr = strtok(NULL, "\"");
            //printf("UID: %s\n", tempStr);
            strcpy(evt->UID, tempStr);
        }
        if (strcmp(tagStr, "startDT") == 0) {
            tagStr = strtok(NULL, "\"");
            tempStr = strtok(NULL, "\"");
            //printf("startDT: %s\n", tempStr);
            evt->startDateTime = makeDate(tempStr);
        }
        if (strcmp(tagStr, "dtStamp") == 0) {
            tagStr = strtok(NULL, "\"");
            tempStr = strtok(NULL, "\"");
            //printf("dtStamp: %s\n", tempStr);
            evt->creationDateTime = makeDate(tempStr);
        }
        if (strcmp(tagStr, "summary") == 0) {
            tagStr = strtok(NULL, "\"");
            tempStr = strtok(NULL, "\"");
            Property* p = makeProperty("SUMMARY", tempStr);
            insertBack(evt->properties, p);
        }
        tagStr = strtok(NULL, "\"");
    }
    Property* p = makeProperty("SUMMARY", summaryString);
    insertBack(evt->properties, p);
    free(creationDate);
    free(startDate);
    free(summaryString);
    if (strlen(evt->UID) == 0 || strcmp("\"}", evt->UID) == 0) {
        deleteEvent(evt);
        return NULL;
    }
    //evt->UID[strlen(evt->UID) - 2] = '\0';
    return evt;
}

void addEvent(Calendar* cal, Event* toBeAdded) {
    if (cal == NULL || toBeAdded == NULL) {
        return;
    }
    
    insertBack(cal->events, toBeAdded);
}

char* printError(ICalErrorCode err) {
    switch(err) {
        case 0:
            return "OK";
            break;
        case 1:
            return "INV_FILE";
            break;
        case 2:
            return "INV_CAL";
            break;
        case 3:
            return "INV_VER";
            break;
        case 4:
            return "DUP_VER";
            break;
        case 5:
            return "INV_PRODID";
            break;
        case 6:
            return "DUP_PRODID";
            break;
        case 7:
            return "INV_EVENT";
            break;
        case 8:
            return "INV_DT";
            break;
        case 9:
            return "INV_ALARM";
            break;
        case 10:
            return "WRITE_ERROR";
            break;
        case 11:
            return "OTHER_ERROR";
            break;
        default:
            return "NA";
            break;
    }
}

void deleteCalendar(Calendar* obj) {
    if (obj == NULL) {
        return;
    }
    freeList(obj->properties);
    obj->properties = NULL;
    freeList(obj->events);
    obj->events = NULL;
    free(obj);
}

//toString
char* printCalendar(const Calendar* obj) {
    char *cal = malloc(100);
    strncpy(cal, "print calendar lol\n", 100);
    return cal;
}

void deleteEvent(void* toBeDeleted) {
    if (toBeDeleted == NULL) {
        return;
    }
    Event* event = (Event*)toBeDeleted;
    freeList(event->alarms);
    freeList(event->properties);
    free(event);
}

int compareEvents(const void* first, const void* second) {
    Event* ev1 = (Event*)first;
    Event* ev2 = (Event*)second;
    return(strcmp(ev1->startDateTime.time, ev2->startDateTime.time));
}

char* printEvent(void* toBePrinted) {
    Event* event = (Event*)toBePrinted;
    int bufferLength = (int)(strlen(event->creationDateTime.date) + strlen(event->creationDateTime.time) + strlen(event->UID) + 11);
    char* buffer = malloc(bufferLength);
    snprintf(buffer, bufferLength, "Date:%s\n%s\n%s", event->creationDateTime.date, event->creationDateTime.time, event->UID);
    return buffer;
}

void deleteAlarm(void* toBeDeleted) {
    if (toBeDeleted == NULL) {
        return;
    }
    Alarm* alarm = (Alarm*)toBeDeleted;
    freeList(alarm->properties);
    if (alarm->trigger != NULL || strlen(alarm->trigger) > 0) {
        free(alarm->trigger);
    }
    free(alarm);
}

int compareAlarms(const void* first, const void* second) {
    Alarm* firstAlarm = (Alarm*)first;
    Alarm* secondAlarm = (Alarm*)second;
    return strcmp(firstAlarm->action, secondAlarm->action);
}

char* printAlarm(void* toBePrinted) {
    Alarm* alarm = (Alarm*)toBePrinted;
    char* trigger = malloc(strlen(alarm->trigger) + 1);
    strncpy(trigger, alarm->trigger, strlen(alarm->trigger)+1);
    return trigger;
}

void deleteProperty(void* toBeDeleted) {
    Property* p = (Property*)toBeDeleted;
    free(p);
}

int compareProperties(const void* first, const void* second) {
    Property* prop1 = (Property*)first;
    Property* prop2 = (Property*)second;
    return (strcmp(prop1->propDescr, prop2->propDescr));
}

char* printProperty(void* toBePrinted) {
    Property* prop = (Property*)toBePrinted;
    char* printableProperty = malloc(strlen(prop->propName) + strlen(prop->propDescr) + 1);
    strncpy(printableProperty, prop->propName, 200);
    strncat(printableProperty, prop->propDescr, strlen(prop->propDescr) + 1);
    return printableProperty;
}

void deleteDate(void* toBeDeleted) {
    DateTime* dt = (DateTime*)toBeDeleted;
    free(dt);
}
int compareDates(const void* first, const void* second) {
    DateTime* dtFirst = (DateTime*)first;
    DateTime* dtSecond = (DateTime*)second;
    return (strcmp(dtFirst->date, dtSecond->date));
}
char* printDate(void* toBePrinted) {
    DateTime* dt = (DateTime*)toBePrinted;
    int bufferLength = (int)(strlen(dt->date) + strlen(dt->time)+10);
    char* buffer = malloc(bufferLength);
    snprintf(buffer, bufferLength, "Date:%s\n%s\n", dt->date, dt->time);
    return buffer;
}

char* substring(char* string, int from, int to) {
    int segmentLength = to - from + 1;
    char* substring = malloc(segmentLength + 1);
    strncpy(substring, string+(from), segmentLength);
    substring[segmentLength] = '\0';
    return substring;
}
