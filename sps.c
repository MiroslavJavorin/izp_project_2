#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

//region macros
/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */
#define CHECK_EXIT_CODE_RUN_PROGRAM_FUNCTION if(exit_code > 0)\
{\
    print_error_message(&exit_code);\
    return exit_code;\
}

#define CHECK_EXIT_CODE if(*exit_code > 0){ return; }
//endregion

//region enums
enum erorrs
{
    ALLOCATING_ERROR = 1,
    UNSUPPORTED_SEPARATORS_ERROR,
    TOO_FEW_ARGS_ERROR
};
//endregion

/* contains information about delim string */ // TODO generalize
typedef struct seps_t
{
    int seps_c;    /* number of separators user entered in delim string in commandline */
    char *seps_v;   /* dynamically allocated array of chars that contains separators */
} seps_t;

//region structures
/* contains information about arguments user entered in commandline */
typedef struct
{
    seps_t seps;
} clargs_t;

/* contains information about a table */
typedef struct
{
    int param1;
    int param2;
} table_t;
//endregion

/* frees all memory allocated by a table_t structure s */
void free_table(table_t *s)
{
    (void) s;
}

/* frees all memory allocated by a clargs_t structure s */
void free_clargs(clargs_t *s)
{
    free(s->seps.seps_v);
}

/**
 *  Adds an item to an array if this item is not represented in the array
 *
 * @param set An array where to add an item
 * @param size Size of an array where to add an item. If item is adds size will be increased
 * @param item A character to add to an array
 * @param exit_code Exit code is changed only if the error while allocating memory has been occurred
 *
 * @return
 *         true, if item has been added to an array
 *         false, if item is already in the array or if error while allocating memory has been occurred
 */
bool set_add_item(seps_t *set, char item, int *exit_code)
{
#ifdef SEPBUG
    printf("line %d in %s -> size     =%d\n", __LINE__, __FUNCTION__, set->seps_c);
#endif
    for(int i = 0; i < set->seps_c; i++)
    {
        printf("i =%d, size =%d, set[i] =%d\n", i , set->seps_c, set->seps_v[i]);
        if(set->seps_v[i] == item)
            return false;
    }
#ifdef MEMBUG
    printf("line %d in %s -> size bef =%d\n",__LINE__, __FUNCTION__, set->seps_c);
#endif

    set->seps_v = (char *)realloc(set->seps_v, (unsigned long)++set->seps_c * sizeof(char));
    if(set->seps_v == NULL)
    {
        *exit_code = ALLOCATING_ERROR;
        return false;
    }
#ifdef MEMBUG
    printf("line %d in %s -> size aft =%d\n",__LINE__, __FUNCTION__, set->seps_c);
#endif
    set->seps_v[set->seps_c-1] = item;
    return true;
}

/**
 * Initializes an array of entered separators(DELIM)
 *
 * @param argc Number of commandline arguments
 * @param argv An array with commandline arguments
 * @param exit_code exit code to change if an error occurred
 */
void init_separators(int argc, char **argv, clargs_t *clargs, int *exit_code)
{
    //region variables
    clargs->seps.seps_c = 0;
    clargs->seps.seps_v = NULL;
    char* sepsptr = NULL;
    int k = 0;
    //endregion

    /* if there is only name, functions and filename */
    if(argc > 3 && strcmp(argv[1], "-d"))
    {
        set_add_item(&(clargs->seps), ' ', exit_code);
        CHECK_EXIT_CODE
    }

    /* if user entered -d flag and there are function names and filename in the commandline */
    else if(argc > 4 && !strcmp(argv[1], "-d"))
    {
        while(argv[2][k])
        {
            /* checks if the user has not entered characters that will lead to undefined program behavior */
            if(argv[2][k] == 10 || argv[2][k] == 92 || argv[2][k] == 39 || argv[2][k] == 34)
            {
                *exit_code = UNSUPPORTED_SEPARATORS_ERROR;
                return;
            }
            set_add_item(&(clargs->seps), argv[2][k], exit_code);
            CHECK_EXIT_CODE
            k++;
        }
    }

    /* If number of arguments is less than or equal to 2 which means t
     * here are only name of the program and filename (optional) */
    else if(argc <= 4)
    {
        *exit_code = TOO_FEW_ARGS_ERROR;
        return;
    }
    clargs->seps.seps_v = sepsptr;
}

void clargs_init(int argc, char **argv, clargs_t* clargs, int *exit_code)
{
    init_separators(argc, argv, clargs, exit_code);
    CHECK_EXIT_CODE
}

void process_table(clargs_t *clargs, table_t *table, int *exit_code)
{
    (void) clargs;
    (void) exit_code;
    (void) table;
}

void print_error_message(int *exit_code)
{
    /* an array with all error messages */
    char *error_msg[] =
    {
            "Twenty afternoons in utopia",
            "Entered separators are not supported buy the program",
            "You've entered too few arguments"
    };

    /* print an error message to stderr */
    fprintf(stderr, "Error %d: %s\n", *exit_code, error_msg[*exit_code - 1]);
}

/**
 * Runs the program: initializes structures, then initializes commandline arguments, then processes the table
 * After every initializing checks for an exit_code to return it if the error has been occurred
 * Frees memory that was allocated while program was running.
 */
int run_program(int argc, char **argv)
{
    //region arguments
    int exit_code = 0;
    table_t  table;
    clargs_t clargs;
    //endregion

    /* initialize all commandline arguments and add them to the structure to use when processing table */
    clargs_init(argc, argv, &clargs, &exit_code);
    CHECK_EXIT_CODE_RUN_PROGRAM_FUNCTION

    /* process the table with the given cl arguments */
    process_table(&clargs, &table, &exit_code);
    CHECK_EXIT_CODE_RUN_PROGRAM_FUNCTION

    free_table(&table);
    free_clargs(&clargs);
    /* exit_code is -1 means table has been processed successfully */
    return (exit_code == -1) ? 0 : exit_code;
}

/* Prints documentation, Author */
void print_documentation()
{
    printf("Kiss an ugly turtle to make it cry");
}

int main(int argc, char **argv)
{
    /* print documentation */
    if(argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        print_documentation();
        return 0;
    }

    /* run the program */
    return run_program(argc, argv);
}
