#define _POSIX_C_SOURCE 200809L
#include "VCParser.h"
#include "LinkedListAPI.h"
#include "VCHelpers.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

VCardErrorCode createCard(char *fileName, Card **obj)
{
    if (fileName == NULL || obj == NULL)
    {
        return INV_FILE;
    }

    /////////////////////////////////////////////////////////////

    // Check file extension (case-insensitive and must be at the end)
    size_t len = strlen(fileName);
    if (len <= 4 || (strcasecmp(fileName + len - 4, ".vcf") != 0 && (len <= 6 || strcasecmp(fileName + len - 6, ".vcard") != 0)))
    {
        return INV_FILE;
    }
    /////////////////////////////////////////////////////////////

    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        return INV_FILE;
    }

    /////////////////////////////////////////////////////////

    Card *newCard = (Card *)malloc(sizeof(Card));
    if (newCard == NULL)
    {
        fclose(file);
        return OTHER_ERROR;
    }

    newCard->fn = NULL;
    newCard->optionalProperties = initializeList(propertyToString, deleteProperty, compareProperties);
    newCard->birthday = NULL;
    newCard->anniversary = NULL;

    if (newCard->optionalProperties == NULL)
    {
        free(newCard);
        fclose(file);
        return OTHER_ERROR;
    }

    /////////////////////////////////////////////////////////

    char line[1024];
    char buffer[1024] = "";
    bool fnFound = false;
    bool beginFound = false;
    bool versionFound = false;
    bool endFound = false;
    bool isFirstLine = true;

    while (fgets(line, sizeof(line), file) != NULL)
    {

        size_t len = strlen(line);

        if (len < 2 || line[len - 1] != '\n' || line[len - 2] != '\r')
        {
            fclose(file);
            return INV_CARD; // Error code 2
        }

        line[len - 2] = '\0';

        // Check for BEGIN:VCARD (must be first line)
        if (isFirstLine)
        {
            if (strcmp(line, "BEGIN:VCARD") != 0)
            {
                fclose(file);
                return INV_CARD;
            }
            beginFound = true;
            isFirstLine = false;
            continue; // Skip further processing for BEGIN line
        }

        // Check for VERSION:4.0 (must appear early)
        if (!versionFound && strcmp(line, "VERSION:4.0") == 0)
        {
            versionFound = true;
            continue; // Skip processing for VERSION line
        }

        // Check for END:VCARD (must be last line)
        if (strcmp(line, "END:VCARD") == 0)
        {
            endFound = true;
            break; // Stop processing after END line
        }

        if (line[0] == ' ' || line[0] == '\t')
        {
            strncat(buffer, line + 1, sizeof(buffer) - strlen(buffer) - 1);
        }
        else
        {
            if (strlen(buffer) > 0)
            {
                VCardErrorCode err = createCardHelper(buffer, newCard, &fnFound);
                if (err != OK)
                {
                    return err;
                }
            }
            strncpy(buffer, line, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
        }
    }

    if (strlen(buffer) > 0)
    {
        VCardErrorCode err = createCardHelper(buffer, newCard, &fnFound);
        if (err != OK)
        {
            return err;
        }
    }

    fclose(file);

    if (!beginFound || !versionFound || !endFound || !fnFound)
    {
        deleteCard(newCard);
        *obj = NULL;
        return INV_CARD;
    }

    *obj = newCard;

    return OK;
}

void deleteCard(Card *obj)
{
    // Free all allocated memory for Card and its components
    // Implement this as per the detailed requirements later

    if (obj == NULL)
    {
        return;
    }

    if (obj->fn != NULL)
    {
        deleteProperty(obj->fn);
    }

    if (obj->optionalProperties != NULL)
    {
        freeList(obj->optionalProperties);
    }

    if (obj->birthday != NULL)
    {
        deleteDate(obj->birthday);
    }

    if (obj->anniversary != NULL)
    {
        deleteDate(obj->anniversary);
    }

    free(obj);
}

char *cardToString(const Card *obj)
{

    if (obj == NULL)
    {
        return NULL;
    }

    // Allocate buffer for output
    size_t bufferSize = 8192; // Ensure sufficient space for complex cards
    char *output = malloc(bufferSize);
    if (output == NULL)
    {
        return NULL;
    }
    strcpy(output, "BEGIN:VCARD\nVERSION:4.0\n");

    // Add FN property
    if (obj->fn != NULL)
    {
        strcat(output, "FN:");
        char *fnValue = (char *)getFromFront(obj->fn->values);
        strcat(output, fnValue);
        strcat(output, "\n");
    }

    // Add BDAY
    if (obj->birthday != NULL)
    {
        strcat(output, "BDAY"); // Start property

        // Add VALUE=text parameter if needed
        if (obj->birthday->isText)
        {
            strcat(output, ";VALUE=text");
        }

        strcat(output, ":"); // Add colon separator

        // Add value
        if (obj->birthday->isText)
        {
            strcat(output, obj->birthday->text); // Text value
        }
        else
        {
            strcat(output, obj->birthday->date); // Date value
            if (obj->birthday->UTC)
            {
                strcat(output, "Z"); // Add UTC indicator
            }
        }

        strcat(output, "\n"); // Use CRLF for vCard line endings
    }

    // Add ANNIVERSARY
    if (obj->anniversary != NULL)
    {
        strcat(output, "ANNIVERSARY"); // Start property

        // Add VALUE=text parameter if needed
        if (obj->anniversary->isText)
        {
            strcat(output, ";VALUE=text");
        }

        strcat(output, ":"); // Add colon separator

        // Add value
        if (obj->anniversary->isText)
        {
            strcat(output, obj->anniversary->text); // Text value
        }
        else
        {
            strcat(output, obj->anniversary->date); // Date value
            strcat(output, "T");                    // Date value
            strcat(output, obj->anniversary->time); // Date value
            if (obj->anniversary->UTC)
            {
                strcat(output, "Z"); // Add UTC indicator
            }
        }

        strcat(output, "\n"); // Use CRLF for vCard line endings
    }

    // Add the N property first if it exists (mandatory after FN)
    ListIterator iter = createIterator(obj->optionalProperties);
    Property *prop;
    while ((prop = nextElement(&iter)) != NULL)
    {
        if (strcmp(prop->name, "N") == 0)
        {
            strcat(output, "N:");
            ListIterator valueIter = createIterator(prop->values);
            char *value;
            bool firstValue = true;
            while ((value = (char *)nextElement(&valueIter)) != NULL)
            {
                if (!firstValue)
                {
                    strcat(output, ";");
                }
                strcat(output, value);
                firstValue = false;
            }
            strcat(output, "\n");
            break; // Only one N property is expected
        }
    }

    // Add remaining optional properties (except N, already added)
    iter = createIterator(obj->optionalProperties);
    while ((prop = nextElement(&iter)) != NULL)
    {
        if (strcmp(prop->name, "N") == 0)
        {
            continue; // Skip N, already processed
        }

        strcat(output, prop->name);

        // Add parameters
        ListIterator paramIter = createIterator(prop->parameters);
        Parameter *param;
        while ((param = nextElement(&paramIter)) != NULL)
        {
            strcat(output, ";");
            strcat(output, param->name);
            strcat(output, "=");
            strcat(output, param->value);
        }

        // Add values
        strcat(output, ":");
        ListIterator valueIter = createIterator(prop->values);
        char *value;
        bool firstValue = true;
        while ((value = nextElement(&valueIter)) != NULL)
        {
            if (!firstValue)
            {
                strcat(output, ",");
            }
            strcat(output, value);
            firstValue = false;
        }

        strcat(output, "\n");
    }

    // Add END:VCARD
    strcat(output, "END:VCARD\n");

    return output;
}

char *errorToString(VCardErrorCode err)
{
    char *error;
    // Return a string representation of the error code
    switch (err)
    {
    case OK:
        error = strdup("OK");
        break;
    case INV_FILE:
        error = strdup("Invalid file");
        break;
    case INV_CARD:
        error = strdup("Invalid card");
        break;
    case INV_PROP:
        error = strdup("Invalid property");
        break;
    case INV_DT:
        error = strdup("Invalid DateTime");
        break;
    case WRITE_ERROR:
        error = strdup("Write error");
        break;
    case OTHER_ERROR:
        error = strdup("Other error");
        break;
    default:
        error = strdup("Unknown error");
        break;
    }

    return error;
}

///////////////////////////////////////////////////////////////

void deleteProperty(void *toBeDeleted)
{

    if (toBeDeleted == NULL)
    {
        return;
    }

    Property *newProperty = (Property *)toBeDeleted;

    free(newProperty->name);
    free(newProperty->group);

    if (newProperty->parameters != NULL)
    {
        freeList(newProperty->parameters);
    }

    if (newProperty->values != NULL)
    {
        freeList(newProperty->values);
    }

    free(newProperty);
}

int compareProperties(const void *first, const void *second)
{
    if (first == NULL || second == NULL)
    {
        if (first == NULL && second == NULL)
        {
            return 0; // Both are NULL, treat as "equal"
        }
        return -1; // One is NULL, invalid comparison
    }

    // Cast the input void pointers to Property pointers
    const Property *prop1 = (Property *)first;
    const Property *prop2 = (Property *)second;

    int nameCompare = strcmp(prop1->name, prop2->name);
    if (nameCompare != 0)
    {
        return nameCompare;
    }

    int groupCompare = strcmp(prop1->group, prop2->group);
    if (groupCompare != 0)
    {
        return groupCompare;
    }

    // Compare parameter lists length int paramLength1 = getLength(prop1->parameters);
    int paramLength1 = getLength(prop1->parameters);
    int paramLength2 = getLength(prop2->parameters);
    if (paramLength1 != paramLength2)
    {
        return paramLength1 - paramLength2;
    }

    // Compare value lists length
    int valueLength1 = getLength(prop1->values);
    int valueLength2 = getLength(prop2->values);
    if (valueLength1 != valueLength2)
    {
        return valueLength1 - valueLength2;
    }

    // Compare parameter contents
    ListIterator paramIter1 = createIterator(prop1->parameters);
    ListIterator paramIter2 = createIterator(prop2->parameters);
    Parameter *param1, *param2;
    while ((param1 = nextElement(&paramIter1)) != NULL &&
           (param2 = nextElement(&paramIter2)) != NULL)
    {
        int paramComparison = compareParameters(param1, param2);
        if (paramComparison != 0)
        {
            return paramComparison; // If any parameter differs, return the comparison result
        }
    }

    // Compare value contents
    ListIterator valueIter1 = createIterator(prop1->values);
    ListIterator valueIter2 = createIterator(prop2->values);
    char *value1, *value2;
    while ((value1 = nextElement(&valueIter1)) != NULL &&
           (value2 = nextElement(&valueIter2)) != NULL)
    {
        int valueComparison = strcmp(value1, value2);
        if (valueComparison != 0)
        {
            return valueComparison; // If any value differs, return the comparison result
        }
    }

    // If all fields are equal, return 0
    return 0; // Placeholder for testing
}

char *propertyToString(void *prop)
{
    if (prop == NULL)
    {
        return NULL;
    }

    // Cast the input to a Property type
    Property *property = (Property *)prop;

    // Start with a reasonable buffer size
    size_t bufferSize = 256;
    char *buffer = malloc(bufferSize);
    if (buffer == NULL)
    {
        return NULL;
    }
    buffer[0] = '\0'; // Initialize buffer as empty string

    // Add group name if it exists
    if (strlen(property->group) > 0)
    {
        strcat(buffer, property->group);
        strcat(buffer, ".");
    }

    // Add property name
    strcat(buffer, property->name);

    // Add parameters
    ListIterator paramIter = createIterator(property->parameters);
    Parameter *param;
    while ((param = nextElement(&paramIter)) != NULL)
    {
        strcat(buffer, ";");
        strcat(buffer, param->name);
        strcat(buffer, "=");
        strcat(buffer, param->value);
    }

    // Add values
    strcat(buffer, ":");
    ListIterator valueIter = createIterator(property->values);
    char *value;
    bool firstValue = true;
    while ((value = nextElement(&valueIter)) != NULL)
    {
        if (!firstValue)
        {
            strcat(buffer, ",");
        }
        strcat(buffer, value);
        firstValue = false;
    }

    return buffer;
}

////////////////////////////////////////////////////////////////

void deleteParameter(void *toBeDeleted)
{
    if (toBeDeleted == NULL)
    {
        return;
    }

    Parameter *newParameter = (Parameter *)toBeDeleted;

    free(newParameter->name);
    free(newParameter->value);

    free(newParameter);
}

int compareParameters(const void *first, const void *second)
{
    if (first == NULL || second == NULL)
    {
        return -1;
    }

    // Cast the void pointers to Parameter pointers
    const Parameter *param1 = (Parameter *)first;
    const Parameter *param2 = (Parameter *)second;

    int nameCompare = strcmp(param1->name, param2->name);
    if (nameCompare != 0)
    {
        return nameCompare;
    }

    return strcmp(param1->value, param2->value);
}

char *parameterToString(void *param)
{
    if (param == NULL)
    {
        return NULL; // If the parameter is NULL, return NULL
    }

    Parameter *parameter = (Parameter *)param;

    // Allocate enough memory to hold "name=value" with a null-terminator
    size_t length = strlen(parameter->name) + strlen(parameter->value) + 2; // +1 for '=', +1 for '\0'
    char *result = malloc(length);
    if (result == NULL)
    {
        return NULL; // Memory allocation failed
    }

    // Format the parameter as "name=value"
    snprintf(result, length, "%s=%s", parameter->name, parameter->value);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////

void deleteValue(void *toBeDeleted)
{
    if (toBeDeleted == NULL)
    {
        return;
    }

    free((char *)toBeDeleted);
}

int compareValues(const void *first, const void *second)
{
    if (first == NULL && second == NULL)
    {
        return 0;
    }

    if (first == NULL || second == NULL)
    {
        return -1;
    }

    return strcmp((const char *)first, (const char *)second);
}

char *valueToString(void *val)
{
    if (val == NULL)
    {
        return NULL;
    }

    char *value = (char *)val;

    char *result = strdup(value);
    if (result == NULL)
    {
        return NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////

void deleteDate(void *toBeDeleted)
{
    if (toBeDeleted == NULL)
    {
        return;
    }

    DateTime *newDateTime = (DateTime *)toBeDeleted;
    free(newDateTime->date);
    free(newDateTime->time);
    free(newDateTime->text);

    free(newDateTime);
}

int compareDates(const void *first, const void *second)
{
    if (first == NULL || second == NULL)
    {
        return -1;
    }

    const DateTime *d1 = (const DateTime *)first;
    const DateTime *d2 = (const DateTime *)second;

    if (d1->UTC != d2->UTC)
    {
        return (d1->UTC) ? 1 : -1;
    }

    if (d1->isText != d2->isText)
    {
        return (d1->isText) ? 1 : -1;
    }

    if (d1->isText)
    {
        return strcmp(d1->text, d2->text);
    }

    // Compare date values
    int dateCompare = strcmp(d1->date, d2->date);
    if (dateCompare != 0)
    {
        return dateCompare;
    }

    return strcmp(d1->time, d2->time);
}

char *dateToString(void *date)
{
    if (date == NULL)
    {
        return strdup("NULL");
    }

    DateTime *dt = (DateTime *)date;

    // Allocate a buffer for the output string
    size_t bufferSize = 1024;
    char *output = malloc(bufferSize);
    if (output == NULL)
    {
        return NULL;
    }

    // Start building the string
    snprintf(output, bufferSize, "DateTime: [");

    // Add isText field
    if (dt->isText)
    {
        strncat(output, "Text: ", bufferSize - strlen(output) - 1);
        strncat(output, dt->text, bufferSize - strlen(output) - 1);
    }
    else
    {
        // Add date and time
        strncat(output, "Date: ", bufferSize - strlen(output) - 1);
        strncat(output, dt->date, bufferSize - strlen(output) - 1);
        strncat(output, ", Time: ", bufferSize - strlen(output) - 1);
        strncat(output, dt->time, bufferSize - strlen(output) - 1);
    }

    // Add UTC flag
    strncat(output, ", UTC: ", bufferSize - strlen(output) - 1);
    strncat(output, dt->UTC ? "true" : "false", bufferSize - strlen(output) - 1);

    // Close the string
    strncat(output, "]", bufferSize - strlen(output) - 1);

    return output;

    return date;
}
/////////////////////////////////////////////////////////
VCardErrorCode writeCard(const char *fileName, const Card *obj)
{
    if (fileName == NULL || obj == NULL)
        return WRITE_ERROR;

    size_t len = strlen(fileName);
    if (len <= 4 ||
        (strcasecmp(fileName + len - 4, ".vcf") != 0 &&
         (len <= 6 || strcasecmp(fileName + len - 6, ".vcard") != 0)))
        return INV_FILE;

    FILE *file = fopen(fileName, "w");
    if (file == NULL)
        return INV_FILE;

    // Write vCard header and version
    fprintf(file, "BEGIN:VCARD\r\nVERSION:4.0\r\n");

    // Write the FN field (must exist)
    if (obj->fn && obj->fn->values && getLength(obj->fn->values) > 0)
    {
        fprintf(file, "FN:%s\r\n", (char *)getFromFront(obj->fn->values));
    }
    else
    {
        fclose(file);
        return INV_CARD;
    }

    if (obj->birthday != NULL)
    {
        fprintf(file, "BDAY");
        if (obj->birthday->isText)
        {
            fprintf(file, ";VALUE=text:");
            fprintf(file, "%s", obj->birthday->text);
        }
        else
        {
            fprintf(file, ":");
            if (obj->birthday->date && strlen(obj->birthday->date) > 0)
                fprintf(file, "%s", obj->birthday->date);
            if (obj->birthday->time && strlen(obj->birthday->time) > 0)
            {
                fprintf(file, "T");
                fprintf(file, "%s", obj->birthday->time);
            }
            if (obj->birthday->UTC)
                fprintf(file, "Z");
        }
        fprintf(file, "\r\n");
    }
    if (obj->anniversary != NULL)
    {
        fprintf(file, "ANNIVERSARY");
        if (obj->anniversary->isText)
        {
            fprintf(file, ";VALUE=text:");
            fprintf(file, "%s", obj->anniversary->text);
        }
        else
        {
            fprintf(file, ":");
            if (obj->anniversary->date && strlen(obj->anniversary->date) > 0)
                fprintf(file, "%s", obj->anniversary->date);
            if (obj->anniversary->time && strlen(obj->anniversary->time) > 0)
            {
                fprintf(file, "T");
                fprintf(file, "%s", obj->anniversary->time);
            }
            if (obj->anniversary->UTC)
                fprintf(file, "Z");
        }
        fprintf(file, "\r\n");
    }

    // Iterate through optional properties.
    // When the "N" property is printed, immediately follow it with birthday and anniversary lines.
    ListIterator iter = createIterator(obj->optionalProperties);
    Property *prop;
    while ((prop = nextElement(&iter)) != NULL)
    {
        // Print group if present.
        if (prop->group && strlen(prop->group) > 0)
            fprintf(file, "%s.", prop->group);
        fprintf(file, "%s", prop->name);

        // Print parameters
        ListIterator paramIter = createIterator(prop->parameters);
        Parameter *param;
        while ((param = nextElement(&paramIter)) != NULL)
        {
            fprintf(file, ";%s=%s", param->name, param->value);
        }
        fprintf(file, ":");

        // Print property values (use comma for GEO; semicolon for others)
        ListIterator valueIter = createIterator(prop->values);
        char *value;
        bool firstValue = true;
        while ((value = nextElement(&valueIter)) != NULL)
        {
            if (!firstValue)
            {
                if (strcasecmp(prop->name, "GEO") == 0 || strcasecmp(prop->name, "NOTE") == 0 || strcasecmp(prop->name, "CATEGORIES") == 0)
                    fprintf(file, ",");
                else
                    fprintf(file, ";");
            }
            fprintf(file, "%s", value);
            firstValue = false;
        }
        fprintf(file, "\r\n");
    }

    fprintf(file, "END:VCARD\r\n");
    fclose(file);
    return OK;
}

VCardErrorCode validateCard(const Card *obj)
{
    if (obj == NULL)
        return INV_CARD;

    // Validate FN field exists and has at least one value.
    if (obj->fn == NULL || obj->fn->values == NULL || getLength(obj->fn->values) == 0)
        return INV_CARD;

    if (obj->optionalProperties == NULL)
        return INV_CARD;

    int kindCount = 0;
    ListIterator propIter = createIterator(obj->optionalProperties);
    Property *prop;
    while ((prop = nextElement(&propIter)) != NULL)
    {
        if (prop->name == NULL || strlen(prop->name) == 0)
            return INV_PROP;

        // VERSION must not appear in optionalProperties; if it does, return INV_CARD (error code 2).
        if (strcasecmp(prop->name, "VERSION") == 0)
            return INV_CARD;

        // BDAY and ANNIVERSARY should not appear in optionalProperties.
        if (strcasecmp(prop->name, "BDAY") == 0 || strcasecmp(prop->name, "ANNIVERSARY") == 0)
            return INV_DT;

        if (strcasecmp(prop->name, "KIND") == 0)
            kindCount++;

        if (prop->values == NULL || getLength(prop->values) == 0)
            return INV_PROP;
        if (prop->parameters == NULL)
            return INV_PROP;

        ListIterator paramIter = createIterator(prop->parameters);
        Parameter *param;
        while ((param = nextElement(&paramIter)) != NULL)
        {
            if (param->name == NULL || strlen(param->name) == 0 ||
                param->value == NULL || strlen(param->value) == 0)
                return INV_PROP;
        }
    }
    if (kindCount > 1)
        return INV_PROP; // Multiple KIND properties (error code 3)

    // Validate DateTime fields for birthday.
    // For text DateTime: date and time must be empty, UTC must be false, and text must be non-empty.
    // For structured DateTime: text must be empty and at least one of date or time must be non-empty.
    if (obj->birthday != NULL)
    {
        if (obj->birthday->isText)
        {
            if ((obj->birthday->date && strlen(obj->birthday->date) > 0) ||
                (obj->birthday->time && strlen(obj->birthday->time) > 0) ||
                obj->birthday->UTC)
                return INV_DT;
            if (obj->birthday->text == NULL || strlen(obj->birthday->text) == 0)
                return INV_DT;
        }
        else
        {
            if (obj->birthday->text && strlen(obj->birthday->text) > 0)
                return INV_DT;
            if ((obj->birthday->date == NULL || strlen(obj->birthday->date) == 0) &&
                (obj->birthday->time == NULL || strlen(obj->birthday->time) == 0))
                return INV_DT;
        }
    }
    // Validate DateTime fields for anniversary similarly.
    if (obj->anniversary != NULL)
    {
        if (obj->anniversary->isText)
        {
            if ((obj->anniversary->date && strlen(obj->anniversary->date) > 0) ||
                (obj->anniversary->time && strlen(obj->anniversary->time) > 0) ||
                obj->anniversary->UTC)
                return INV_DT;
            if (obj->anniversary->text == NULL || strlen(obj->anniversary->text) == 0)
                return INV_DT;
        }
        else
        {
            if (obj->anniversary->text && strlen(obj->anniversary->text) > 0)
                return INV_DT;
            if ((obj->anniversary->date == NULL || strlen(obj->anniversary->date) == 0) &&
                (obj->anniversary->time == NULL || strlen(obj->anniversary->time) == 0))
                return INV_DT;
        }
    }

    return OK;
}