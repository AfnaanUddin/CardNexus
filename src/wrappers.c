// MOHAMMED AFNAAN UDDIN
// 1269872

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VCParser.h"

// Wrapper function to validate a vCard
int validate_vcard(char *filename) {
    Card *card = NULL;
    VCardErrorCode result = createCard(filename, &card);
    if (result != OK) {
        return result; // Return error code if invalid
    }

    result = validateCard(card);
    deleteCard(card); // Free memory
    return result;
}

// Wrapper function to get the full name (FN property) from a vCard
char* get_vcard_name(char *filename) {
    Card *card = NULL;
    VCardErrorCode result = createCard(filename, &card);
    if (result != OK || card->fn == NULL) {
        return NULL; // Return NULL if file is invalid
    }

    char *name = malloc(strlen(card->fn->values->head->data) + 1);
    strcpy(name, card->fn->values->head->data);

    deleteCard(card); // Free memory
    return name;
}

// Wrapper function to write a vCard back to a file
int save_vcard(const char *filename, const Card *card) {
    return writeCard(filename, card);
}

int update_vcard_name(char *filename, char *new_name) {
    Card *card = NULL;
    VCardErrorCode result = createCard(filename, &card);
    if (result != OK || card->fn == NULL) {
        return result; 
    }
    
    free(card->fn->values->head->data);
    card->fn->values->head->data = malloc(strlen(new_name) + 1);
    strcpy(card->fn->values->head->data, new_name);

    // Validate before saving
    result = validateCard(card);
    if (result != OK) {
        deleteCard(card);
        return result; // Return validation error
    }

    // Save the updated card
    result = writeCard(filename, card);
    deleteCard(card);
    return result;
}

char* get_vcard_details(char *filename) {
    Card *card = NULL;
    VCardErrorCode result = createCard(filename, &card);
    if (result != OK) {
        return NULL;
    }

    char *details = malloc(8192); // Adjust as needed
    snprintf(details, 8192, "File: %s\nName: %s\nBirthday: %s\nAnniversary: %s\nOther Props: %d",
             filename,
             (char *)getFromFront(card->fn->values),
             card->birthday ? dateToString(card->birthday) : "None",
             card->anniversary ? dateToString(card->anniversary) : "None",
             getLength(card->optionalProperties));

    deleteCard(card);
    return details;
}
