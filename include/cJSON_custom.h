// FUNCTION DEFINITION
// General purpose
void    cJSON_add_number_to_array           (cJSON* array, const char* name, double number);
void    cJSON_add_string_to_array           (cJSON* array, const char* name, const char* string);
void    cJSON_replace_number_value          (cJSON* monitor, const char* key, double new_value);

// Tp2 purpose
cJSON*  cJSON_get_header_response_by_server (cJSON** responses_p);
cJSON*  cJSON_get_header_request_by_client  (cJSON** requests_p, const char* client_name);

void    cJSON_add_pid                       (cJSON* monitor);
