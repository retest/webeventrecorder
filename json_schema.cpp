#pragma once

#include <string>

std::string json_schema = "{	\"$schema\": \"http://json-schema.org/draft-04/schema#\",    \"description\": \"Validation for XY Recorder\",    \"type\": \"object\",    \"properties\":    {      \"start_time\":		 {\"type\": \"integer\", \"minimum\": 0},      \"start_url\":		 {\"type\": \"string\"},       \"actions\":       {         \"type\": \"array\",         \"items\":         {           \"type\": \"object\",           \"properties\": { \"type\": {\"type\": \"string\"}, \"x\": {\"type\": \"integer\"}, \"y\": {\"type\": \"integer\"}, \"wp\": {\"type\": \"integer\"}, \"time\": {\"type\": \"integer\"}},           \"required\": [\"type\", \"time\"],           \"additionalProperties\": true         }       }    },    \"required\": [\"start_time\", \"start_url\", \"actions\", \"events\"],    \"additionalProperties\": false}";