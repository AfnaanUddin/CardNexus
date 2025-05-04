// MOHAMMED AFNAAN UDDIN
// 1269872

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "LinkedListAPI.h"
#include "VCParser.h"


// Adds a parameter to a property's parameter list, ensuring no duplicates
void addParameter(Property *prop, const char *name, const char *value);

VCardErrorCode createCardHelper(const char *line, Card *newCard, bool *fnFound);
// Checks if a parameter already exists in the parameter list
bool parameterExists(List *parameters, const char *name, const char *value);
