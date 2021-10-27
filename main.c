#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POSSIBLE_NUMBER_OF_STATES 512
#define CHUNK_SIZE 512
#define FIRST_ASCII 48
#define SIZEOF_ASCII_ARRAY 75


typedef enum {
    false, true
} bool;

//Transition

struct s_transition {
    int curr_state;
    char char_read;
    char char_write;
    char movement;
    int next_state;
    struct s_transition *next;
};

struct char_input {
    char letter;
    struct s_transition *pointer_array[POSSIBLE_NUMBER_OF_STATES];
};

struct acc_state {
    int final_state;
    struct acc_state *next;
};

//TAPE

struct tape {
    struct tape *next;
    struct tape *prev;
    char chunk[CHUNK_SIZE + 1];
    int chunkNumber;
};

//PIN

struct pin {
    struct pin *next;
    struct tape *pinTape;
    int currentIndex;
    int curr_state;
    bool isNew;
};


//GLOBAL VARIABLES

struct acc_state *acceptance_state;
long int max_operation;
struct char_input arrayInput[SIZEOF_ASCII_ARRAY];
struct pin *pPin;
char *pInput; 
unsigned int totalChunk;
unsigned short sizeOfPartialChunk;

//Population of the array with the possible char used in transitions
void populate_arrayInput(struct char_input *arrayInput) {
    int i;
    for (i = 0; i < SIZEOF_ASCII_ARRAY; i++) {
        arrayInput[i].letter = (char) (FIRST_ASCII + i);
    }
}

int input_operation(char *input) {

    if (strcmp(input, "tr") == 0) {
        return 1;
    } else if (strcmp(input, "acc") == 0) {
        return 2;
    } else if (strcmp(input, "max") == 0) {
        return 3;
    } else if (strcmp(input, "run") == 0) {
        return 4;
    }

    return 0;    //it's an instruction
}

struct s_transition *transition(char *pComando) {
    struct s_transition *new_transition = malloc(sizeof(struct s_transition));
    char k;
    int state;

    sscanf(pComando, "%d", &state);       //converts the number contained in the char array in an int

    new_transition->curr_state = state;
    scanf("%c", &k);					//catch autom new line
    scanf("%c %c %c %d", &new_transition->char_read, &new_transition->char_write, &new_transition->movement,
          &new_transition->next_state);
    new_transition->next = NULL;

    return new_transition;
}

//Hash function used for the insertion of a new transition
int hash(int state) {
    int key = state % POSSIBLE_NUMBER_OF_STATES;
    return key;
}

//Insertion of the transition with a direct access in the arrayInput (casted char value to an int used as index) and direct access
// with hashfunction to the index of the array of transiontions pointers
// collisions are solved with chaining
void insert_transition(struct s_transition *newTransition) {
    struct s_transition *pointer = arrayInput[((int) newTransition->char_read) - FIRST_ASCII].pointer_array[hash(
            newTransition->curr_state)];
    bool isNearInsert = false;

    if (pointer == NULL) {      //first element
        arrayInput[((int) newTransition->char_read) - FIRST_ASCII].pointer_array[hash(
                newTransition->curr_state)] = newTransition;
    } else {
        while (pointer->next != NULL && isNearInsert == false) {
            if (pointer->curr_state ==
                newTransition->curr_state) { 	//transition with the same curr_state are insert close to each other
                newTransition->next = pointer->next;
                isNearInsert = true;
            } else {
                pointer = pointer->next;
            }
        }
        pointer->next = newTransition;
    }

}

void insert_acceptance_state(char *pComando) {
    struct acc_state *newState = malloc(sizeof(struct acc_state));
    int state;

    sscanf(pComando, "%d", &state);       //converts the number contained in the char array in an int

    newState->final_state = state;
    newState->next = NULL;

    if (acceptance_state == NULL) {
        acceptance_state = newState;
    } else {
        struct acc_state *support_pointer = acceptance_state;
        while (support_pointer->next != NULL) {
            support_pointer = support_pointer->next;
        }
        support_pointer->next = newState;
    }
}

//Get input from stdin char by char, reallocate if the input string doesn't fit in the currentSize allocated
char *inputString() {
    unsigned int currentSize = 512;
    char *fileInput = malloc(sizeof(char) * currentSize);
    int currentChar;
    unsigned int lengthOfInput = 0;

    currentChar = fgetc(stdin);

    if (currentChar == EOF) {                         //check if the input file is at the end
        return strcpy(fileInput, "EndOfFile");
    }

    while (currentChar != EOF && currentChar != '\n') {
        fileInput[lengthOfInput] = (char) currentChar;
        lengthOfInput++;

        if (lengthOfInput == currentSize) {				//needs realloc
            currentSize = currentSize * 2;
            fileInput = realloc(fileInput, sizeof(char) * (currentSize));		
        }

        currentChar = fgetc(stdin);
    }

    fileInput[lengthOfInput] = '\0';
    totalChunk = (lengthOfInput - 1) / CHUNK_SIZE;   // number of chunk componing the input string, starting from zero
    
    return realloc(fileInput, sizeof(char) * (lengthOfInput + 1));   //resize to the correct dimension of the input
}


//dynamic allocation of the struct tape which contains a chunk of the input string
struct tape *createChunkFromInput(struct tape *chunk, int chunkNumber) {

    struct tape *newChunk = malloc(sizeof(struct tape));  
    unsigned short i = 0;
    int currentChunkNumber = chunkNumber + 1;

    int pos = CHUNK_SIZE * currentChunkNumber;			//starting index of the new chunk string to copy from the input string

    while (i < CHUNK_SIZE && pInput[pos] != '\0') {		//copy of the input-chunk from the input string
        newChunk->chunk[i] = pInput[pos];
        i++;
        pos++;
    }

    newChunk->chunk[i] = '\0';

    newChunk->next = NULL;
    newChunk->prev = chunk;
    newChunk->chunkNumber = currentChunkNumber;

    if(i < CHUNK_SIZE){
        sizeOfPartialChunk = i;
    }

    return newChunk;
}


//dynamic allocation of the struct tape which contains the first chunk of the input string
struct tape *create_copy_tape_fromArrayHEAD() {
    struct tape *newChunk = malloc(sizeof(struct tape));  	
    unsigned short i = 0;

    while (i < CHUNK_SIZE && pInput[i] != '\0') {		//copy of the input-chunk from the input string
        newChunk->chunk[i] = pInput[i];
        i++;
    }
    newChunk->chunk[i] = '\0';

    newChunk->next = NULL;
    newChunk->prev = NULL;
    newChunk->chunkNumber = 0;

    if(i < CHUNK_SIZE){
        sizeOfPartialChunk = i;
    }

    return newChunk;
}

//dynamic allocation of the struct pin used to analyze the tape
void firstPin() {
    struct pin *newPin = malloc(sizeof(struct pin));
    newPin->curr_state = 0;
    newPin->pinTape = NULL;
    newPin->next = NULL;
    newPin->isNew = false;

    newPin->pinTape = create_copy_tape_fromArrayHEAD();
    newPin->currentIndex = 0;

    if (pPin == NULL) {
        pPin = newPin;     //head insertion
    } else {
        printf("Error, pPin head is not NULL");
    }
}

//Duplication of the tape
struct tape *duplicateTape(struct tape *tapeToClone) {
    struct tape *tapeToCloneCopy = tapeToClone;
    struct tape *headTape;
    int i;
    int currentChunkPos = 0;

    while (tapeToCloneCopy->prev != NULL) {            //goes backwards until reaches the first el of the list
        tapeToCloneCopy = tapeToCloneCopy->prev;
        currentChunkPos++;
    }

    headTape = malloc(sizeof(struct tape));  //head of the list of the input string
    memcpy(headTape->chunk, tapeToCloneCopy->chunk, CHUNK_SIZE);		//copy of the chunk string

    headTape->chunk[CHUNK_SIZE] = '\0';
    headTape->next = NULL;
    headTape->prev = NULL;
    headTape->chunkNumber = tapeToCloneCopy->chunkNumber;

    struct tape *lastChunk = headTape;       //head copy
    struct tape *newTape;

    tapeToCloneCopy = tapeToCloneCopy->next;     //pTapeCopy goes to the next cell

    while (tapeToCloneCopy != NULL) {
        newTape = malloc(sizeof(struct tape));
        memcpy(newTape->chunk, tapeToCloneCopy->chunk, CHUNK_SIZE);			
        newTape->chunk[CHUNK_SIZE] = '\0';

        newTape->next = NULL;
        newTape->prev = lastChunk;
        newTape->chunkNumber = tapeToCloneCopy->chunkNumber;
        lastChunk->next = newTape;

        tapeToCloneCopy = tapeToCloneCopy->next;		        //pTapeCopy goes to the next cell
        lastChunk = lastChunk->next;
    }

    for (i = 0; i < currentChunkPos; i++) {              //starting position of the pin
        headTape = headTape->next;
    }

    return headTape;
}

//Duplication of the struct pin 
struct pin *duplicatePin(struct pin *pinToClone) {
    struct pin *newPin = malloc(sizeof(struct pin));
    newPin->curr_state = pinToClone->curr_state;
    newPin->next = NULL;
    newPin->isNew = true;

    newPin->pinTape = duplicateTape(pinToClone->pinTape);
    newPin->currentIndex = pinToClone->currentIndex;

    if (pPin == NULL) {
        pPin = newPin;     //head insertion
    } else {
        struct pin *supportPointer = pPin;

        while (supportPointer->next != NULL) {
            supportPointer = supportPointer->next;
        }

        supportPointer->next = newPin;
    }

    return newPin;
}

//applies the transition to the tape, writes the char, moves the pin, if it goes over the string limits (right or left) 
// it's dynamically allocated a struct tape with '_' in the chunk array, because the tape is bi-infinite, and sets the new state
struct tape *apply_transition(struct pin *pin, struct s_transition *transition) {
    struct tape *blankTape = NULL;

    pin->pinTape->chunk[pin->currentIndex] = transition->char_write;

    if (transition->movement == 'R') {
        pin->currentIndex++;

        if (pin->pinTape->chunk[pin->currentIndex] == '\0') {			//end of chunk string reached
            if (pin->pinTape->next == NULL) {

                if (pin->pinTape->chunkNumber < totalChunk) {			//create new chunk from the input string
                    pin->pinTape->next = createChunkFromInput(pin->pinTape, pin->pinTape->chunkNumber);

                } else {											//need to allocate blank chunk
                    blankTape = malloc(sizeof(struct tape));

                    memset(blankTape->chunk, '_', CHUNK_SIZE);
                    blankTape->chunk[CHUNK_SIZE] = '\0';

                    blankTape->chunkNumber = (pin->pinTape->chunkNumber) + 1;
                    blankTape->next = NULL;
                    blankTape->prev = pin->pinTape;
                    pin->pinTape->next = blankTape;
                }
            }
            pin->pinTape = pin->pinTape->next;		 //pin move
            pin->currentIndex = 0;
        }

    } else if (transition->movement == 'L') {
        pin->currentIndex--;

        if (pin->currentIndex == -1) {              //previous of the first element of the chunk reached
            if (pin->pinTape->prev == NULL) {   //need to allocate blank chunk

                blankTape = malloc(sizeof(struct tape));
                memset(blankTape->chunk, '_', CHUNK_SIZE);
                blankTape->chunk[CHUNK_SIZE] = '\0';
                blankTape->chunkNumber = (pin->pinTape->chunkNumber) - 1;
                blankTape->next = pin->pinTape;
                blankTape->prev = NULL;

                pin->pinTape->prev = blankTape;
            }

            pin->currentIndex = CHUNK_SIZE - 1;     //last char of the chunk array, before \0
            pin->pinTape = pin->pinTape->prev; 		//pin move

            if(pin->pinTape->chunk[sizeOfPartialChunk] == '\0'){	//correct set of the index if the chunk is not complete 
                pin->currentIndex = sizeOfPartialChunk - 1;    		//(the last one, in case the division of the input in chunk is not % 0)
            }
        }

    } else if (transition->movement != 'S') {
        printf("ERROR\n");
    }

    pin->curr_state = transition->next_state;		

    return pin->pinTape;
}

//check if the state analyzed is a final one and returns a boolean result
bool isFinal(int curr_state) {
    struct acc_state *supportPointer = acceptance_state;

    while (supportPointer != NULL) {
        if (supportPointer->final_state == curr_state) {
            return true;
        }
        supportPointer = supportPointer->next;
    }
    return false;
}


struct tape *remove_tape(struct tape *head) {

    while (head->prev != NULL) {
        head = head->prev;         //to the head of the list
    }

    struct tape *prec = head;

    while (head != NULL) {
        head = head->next;
        free(prec);
        prec = head;
    }

    return head;
}

//Removal of the dynamics structs used to analyze the input string
void remove_pin() {
    struct pin *supportPointer = pPin;

    while (pPin != NULL) {
        pPin->pinTape = remove_tape(supportPointer->pinTape);
        pPin = pPin->next;
        free(supportPointer);
        supportPointer = pPin;
    }
}

//Removal of a struct pin (and his tape) which is searching for an inexistent transition (curr_state and char_read)
void remove_uselessPin(struct pin *pinToRemove) {
    struct pin *sPointer = pPin;
    struct pin *prec = NULL;

    if (sPointer == pinToRemove) {
        if (sPointer->next != NULL) {
            pPin = sPointer->next;
        } else {
            pPin = NULL;
        }

        pinToRemove->pinTape = remove_tape(pinToRemove->pinTape);
        free(pinToRemove);

    } else {
        while (sPointer != pinToRemove && sPointer != NULL) {
            prec = sPointer;
            sPointer = sPointer->next;
            if (sPointer == NULL) {
                printf("Pin to remove not found\n");
            }
        }

        if (sPointer != NULL) {
            if (sPointer->next != NULL) {
                prec->next = sPointer->next;
            } else {
                prec->next = NULL;
            }

            sPointer->pinTape = remove_tape(sPointer->pinTape);
            free(sPointer);
        }

    }
}

//Analysis of the tape in the current position of the pin, search of the possibles transitions
// if is there more than one transition applicable, it's an undeterminist case, the tape will be cloned as many times as necessary 
// and each transition will be applied, one for tape
bool scan_tape(struct pin *pin) {
    bool foundFirst = false;
    struct s_transition *transition = arrayInput[((int) pin->pinTape->chunk[pin->currentIndex]) - FIRST_ASCII].pointer_array[hash(
            pin->curr_state)];
    struct pin *newPin;
    struct s_transition *firstTransition = NULL;


    while (transition != NULL && !(foundFirst == true && transition->curr_state != pin->curr_state)) {

        if (foundFirst == true) {
            newPin = duplicatePin(pin);

            newPin->pinTape = apply_transition(newPin, transition);

        } else if (transition->curr_state == pin->curr_state) {
            foundFirst = true;
            firstTransition = transition;
        }

        transition = transition->next;
    }

    if (foundFirst == true) {  //first transition found is done at the end to allow the copy of the tape in case of non-determ transitions

        pin->pinTape = apply_transition(pin, firstTransition);

        return true;        //the tape is active

    } else {                  //transition (char_read - curr_ state) doesn't exist
        return false;         ///the tape is inactive, it will be deleted
    }
}

//analysis of the input string, is done one operation for each tape (may be more than one in case of non-determ transitions) 
// until the counter is less than the limit, but it will stop if one arrives in a final state, then the input will be accepted, 
// or if there are no tapes to work on it and then the input will be refused
int analyze_tape() {
    struct pin *supportPointer;
    struct pin *pin_to_delete = NULL;
    int num_of_operations = 0;
    bool isActive = false;
    bool out = false;


    while (num_of_operations <= max_operation && out == false) {     //cycle on all the tapes

        supportPointer = pPin;					//head

        while (supportPointer != NULL) {
            if (supportPointer->isNew == false) {    //a pin added recently has already done his first operation
                isActive = scan_tape(supportPointer);

                if (isActive == false) {                  
                    pin_to_delete = supportPointer;
                } else {
                    if (isFinal(supportPointer->curr_state) == true) {
                        return 1;			//string accepted
                    }
                }
            } else {
                supportPointer->isNew = false;
            }

            supportPointer = supportPointer->next;

            if (pin_to_delete != NULL) {				//tape needs to be deleted
                remove_uselessPin(pin_to_delete);
                pin_to_delete = NULL;
            }
        }

        num_of_operations++;

        if (pPin == NULL) {      //all tapes are ended in a no-final state
            out = true;
        }
    }
    if (out == true) {
        return 0;				//string refused
    }
    return 2;				//string refused, U
}

void output_result(int result) {
    if (result == 2) {
        printf("U\n");
    } else {
        printf("%d\n", result);
    }
}

void null_initializer() {
    int i;
    int j;

    for (i = 0; i < SIZEOF_ASCII_ARRAY; i++) {
        for (j = 0; j < POSSIBLE_NUMBER_OF_STATES; j++) {
            arrayInput[i].pointer_array[j] = NULL;
        }
    }
}

int main(int argc, char *argv[]) {
    char pCommand[50];
    char emptyChar;
    int curr_button = 5;
    int switch_case;
    struct s_transition *newTransition;
    bool isInfo = false;
    bool isRun = false;
    bool isEnd = false;
    bool isFirstInput = false;

    pInput = NULL;

    populate_arrayInput(arrayInput);
    null_initializer();

    while (isEnd == false) {
        if (isRun == false) {

            scanf("%s", pCommand);      //reads until the space ' ' => it's command or an info
            switch_case = input_operation(pCommand);
            if (switch_case != 0) {
                curr_button = switch_case;
                isInfo = false;
                if (curr_button == 4) {
                    isRun = true;
                    isInfo = true;
                }
            } else {
                isInfo = true;
            }
        }
        if (isInfo == true || isRun == true) {          	//reading input strings
            switch (curr_button) {
                case 1:                                		//CASE 1 : transitions
                    newTransition = transition(pCommand);
                    insert_transition(newTransition);

                    break;
                case 2:                                		//CASE 2: acceptances states
                    insert_acceptance_state(pCommand);      //dynamic insertion in a list

                    break;
                case 3:                              		//CASE 3: max
                    sscanf(pCommand, "%ld", &max_operation);

                    break;
                case 4:                             		//CASE 4: run
                    if (isFirstInput == false) {
                        scanf("%c", &emptyChar);       		//scan of an empty new line
                        isFirstInput = true;
                    }

                    pInput = inputString();  	 //reads input

                    if (strcmp(pInput, "EndOfFile") != 0 && strcmp(pInput, "") != 0) {

                        firstPin();     //beginning situation of the string read

                        output_result(analyze_tape());    //algorithm of Turing Machine single tape

                        remove_pin();
                        
                    } else {
                        isEnd = true;                   //EOF
                    }
                    
                    free(pInput);

                    break;
                default:
                    break;
            }
        }
    }

    return 0;
}
