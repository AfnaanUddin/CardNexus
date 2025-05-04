// MOHAMMED AFNAAN UDDIN
// 1269872
#define _POSIX_C_SOURCE 200809L
#include "VCParser.h"
#include "LinkedListAPI.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool parameterExists(List *parameters, const char *name, const char *value)
{
    ListIterator iter = createIterator(parameters);
    Parameter *param;
    while ((param = nextElement(&iter)) != NULL)
    {
        if (strcmp(param->name, name) == 0 && strcmp(param->value, value) == 0)
        {
            return true;
        }
    }
    return false;
}

void addParameter(Property *prop, const char *name, const char *value)
{
    if (!parameterExists(prop->parameters, name, value))
    {
        Parameter *param = malloc(sizeof(Parameter));
        if (param == NULL)
        {
            return;
        }
        param->name = strdup(name);
        param->value = strdup(value);
        insertBack(prop->parameters, param);
    }
}

VCardErrorCode createCardHelper(const char *line, Card *newCard, bool *fnFound)
{
    if (strncmp(line, "FN:", 3) == 0 && !(*fnFound))
    {
        *fnFound = true;

        char *fnValue = strdup(line + 3);
        if (fnValue == NULL)
        {
            return OTHER_ERROR;
        }

        Property *fnProperty = malloc(sizeof(Property));
        if (fnProperty == NULL)
        {
            free(fnValue);
            return OTHER_ERROR;
        }

        fnProperty->name = strdup("FN");
        if (fnProperty->name == NULL)
        {
            free(fnValue);
            free(fnProperty);
            return OTHER_ERROR;
        }

        fnProperty->group = strdup("");
        if (fnProperty->group == NULL)
        {
            free(fnProperty->name);
            free(fnValue);
            free(fnProperty);
            return OTHER_ERROR;
        }

        fnProperty->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
        if (fnProperty->parameters == NULL)
        {
            free(fnProperty->group);
            free(fnProperty->name);
            free(fnValue);
            free(fnProperty);
            return OTHER_ERROR;
        }

        fnProperty->values = initializeList(valueToString, deleteValue, compareValues);
        if (fnProperty->values == NULL)
        {
            freeList(fnProperty->parameters);
            free(fnProperty->group);
            free(fnProperty->name);
            free(fnValue);
            free(fnProperty);
            return OTHER_ERROR;
        }

        insertBack(fnProperty->values, fnValue);
        newCard->fn = fnProperty;
    }
    else if (strstr(line, "BDAY") == line || strstr(line, "ANNIVERSARY") == line)
    {
        DateTime *dt = malloc(sizeof(DateTime));
        if (dt == NULL)
        {
            return OTHER_ERROR;
        }

        dt->UTC = false;
        dt->isText = false;
        dt->date = NULL;
        dt->time = strdup("");
        dt->text = strdup("");

        // Find the colon to separate parameters from value
        char *colon = strchr(line, ':');
        if (colon == NULL || colon == line)
        {
            free(dt->time);
            free(dt->text);
            free(dt);
            return INV_CARD;
        }

        // Extract parameters and check for VALUE=text
        char *paramsAndName = strndup(line, colon - line);
        bool isTextDate = (strstr(paramsAndName, "VALUE=text") != NULL);
        free(paramsAndName);

        char *value = strdup(colon + 1);
        if (value == NULL)
        {
            free(dt->time);
            free(dt->text);
            free(dt);
            return OTHER_ERROR;
        }

        dt->isText = isTextDate; // Set flag based on parameters

        if (dt->isText)
        {
            free(dt->text);
            dt->text = strdup(value);
            dt->date = strdup("");
        }
        else
        {
            // Check if it ends with 'Z' (UTC time)
            size_t len = strlen(value);
            if (len > 0 && value[len - 1] == 'Z')
            {
                dt->UTC = true;
                value[len - 1] = '\0';
            }

            if (strncmp(value, "--", 2) == 0)
            {
                free(dt->time);
                dt->date = strdup(value); // Store `--0203` as-is
                dt->time = strdup("");    // No time
            }
            else
            {
                // Split full DateTime into date and time
                char *T = strchr(value, 'T');
                if (T != NULL)
                {
                    free(dt->time);
                    *T = '\0'; // Split date and time
                    dt->date = strdup(value);
                    dt->time = strdup(T + 1);
                }
                else
                {
                    free(dt->time);
                    dt->date = strdup(value);
                    dt->time = strdup("");
                }
            }
        }



        // Assign to the correct field in `Card`
        if (strstr(line, "BDAY") == line)
        {
            if (newCard->birthday != NULL)
            {
                deleteDate(newCard->birthday);
            }
            newCard->birthday = dt;
        }
        else
        {
            if (newCard->anniversary != NULL)
            {
                deleteDate(newCard->anniversary);
            }
            newCard->anniversary = dt;
        }

        free(value);
    }
    else if (strchr(line, ':') != NULL)
    {
        // Skip header/footer lines
        if (strncmp(line, "BEGIN:VCARD", 11) == 0 ||
            strncmp(line, "VERSION:4.0", 11) == 0 ||
            strncmp(line, "END:VCARD", 9) == 0)
        {
            return OK;
        }
        else if (strncmp(line, "FN", 2) == 0)
        {
            return INV_PROP;
        }

        char *colon = strchr(line, ':');
        char *nameAndParams = strndup(line, colon - line);
        char *value = strdup(colon + 1);

        if (nameAndParams == NULL || value == NULL)
        {
            if(nameAndParams != NULL)
            {
                free(nameAndParams);
            } 
            if(value != NULL)
            {
                free(value);
            }
            return OTHER_ERROR;
        }

        char *group = strdup("");
        char *propertyName = NULL;
        char *dot = strchr(nameAndParams, '.');
        if (dot != NULL)
        {
            *dot = '\0';
            free(group);
            group = strdup(nameAndParams);
            propertyName = (*(dot + 1) != '\0') ? strdup(dot + 1) : strdup("");
        }
        else
        {
            propertyName = strdup(nameAndParams);
        }

        if (propertyName == NULL)
        {
            free(nameAndParams);
            free(value);
            free(group);
            return OTHER_ERROR;
        }

        // Split parameters from propertyName, not nameAndParams
        char *semicolon = strchr(propertyName, ';'); // Key Fix: Use propertyName here
        char *parameters = semicolon ? strdup(semicolon + 1) : strdup("");
        if (semicolon != NULL)
        {
            *semicolon = '\0'; // Truncate propertyName to exclude parameters
        }

        if (strlen(propertyName) == 0)
        {
            free(nameAndParams);
            free(propertyName);
            free(group);
            free(value);
            free(parameters);
            return INV_PROP;
        }

        Property *newProperty = malloc(sizeof(Property));
        if (newProperty == NULL)
        {
            free(nameAndParams);
            free(propertyName);
            free(group);
            free(value);
            free(parameters);
            return OTHER_ERROR;
        }

        newProperty->name = propertyName;
        newProperty->group = group;
        newProperty->parameters = initializeList(parameterToString, deleteParameter, compareParameters);
        newProperty->values = initializeList(valueToString, deleteValue, compareValues);

        char *paramToken = strtok(parameters, ";");
        while (paramToken != NULL)
        {
            char *equalsPos = strchr(paramToken, '=');
            if (equalsPos != NULL)
            {
                *equalsPos = '\0';
                char *value_ = equalsPos + 1;

                if (strcmp(paramToken, "") == 0 || strcmp(value_, "") == 0)
                {
                    free(nameAndParams);
                    free(value);
                    free(parameters);
                    free(value_);
                    return INV_PROP;
                }

                addParameter(newProperty, paramToken, value_);
            }
            else
            {
                if (equalsPos == NULL)
                {
                    free(nameAndParams);
                    free(parameters);
                    // free(paramToken);
                    free(value);
                    return INV_PROP;
                }
            }
            paramToken = strtok(NULL, ";");
        }
        

        char *delimiter = (strcmp(propertyName, "N") == 0 || strcmp(propertyName, "ADR") == 0 || strcmp(propertyName, "TEL") == 0) ? ";" : ",";
        char *start = value;
        char *end;
        size_t delim_len = strlen(delimiter);

        while ((end = strstr(start, delimiter)) != NULL)
        {
            *end = '\0'; // Terminate the current token
            insertBack(newProperty->values, strdup(start));
            start = end + delim_len; // Move past the delimiter
        }
        insertBack(newProperty->values, strdup(start)); // Add the last token

        insertBack(newCard->optionalProperties, newProperty);

        free(nameAndParams);
        free(value);
        free(parameters);
    }

    return OK;
}
