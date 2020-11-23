/**
 * Project 2 - simple spreadsheet editor 2
 * Subject IZP 2020/21
 * @Author Skuratovich Aliaksandr xskura01@fit.vutbr.cz
*/

//region includes
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
//endregion

//region macros
/*
 *  MEMBUG  - debug memory usage
 *  SEPSBUG - debug separators
 * */

/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */
#define CHECK_EXIT_CODE_IN_RUN_PROGRAM if(exit_code > 0)\
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
    TOO_FEW_ARGS_ERROR,
    NO_SUCH_FILE_ERROR
};

//endregion

/* contains information about delim string */ // TODO generalize
typedef struct carr_t
{
    int   elems_c;    /* number of separators user entered in delim string in commandline */
    char *elems_v;   /* dynamically allocated array of chars that contains separators */
} carr_t;

//region structures
/* contains information about arguments user entered in commandline */
typedef struct
{
    carr_t seps;
    FILE  *ptr;
} clargs_t;

/* contains information about a table */
typedef struct
{
    carr_t***     table; /* table contains rows which contain cells */
    unsigned int  row_c; /* number of rows of the table */
    unsigned int* col_c; /* number of columns of each row */
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
    if(s->seps.elems_v)
        free(s->seps.elems_v);
    if(s->ptr)
        fclose(s->ptr);
}

void clear_data(table_t *table, clargs_t *clargs)
{
    free_table(table);
    free_clargs(clargs);
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
bool set_add_item(carr_t *set, char item, int *exit_code)
{
    for(int i = 0; i < set->elems_c; i++)
        if(set->elems_v[i] == item)
            return false;


    set->elems_v = (char *)realloc(set->elems_v, (unsigned long)++set->elems_c * sizeof(char));
    if(set->elems_v == NULL)
    {
        *exit_code = ALLOCATING_ERROR;
        return false;
    }

    set->elems_v[set->elems_c-1] = item;
    return true;
}

/**
 * Initializes an array of entered separators(DELIM)
 *
 * @param argc Number of commandline arguments
 * @param argv An array with commandline arguments
 * @param exit_code exit code to change if an error occurred
 */
void init_separators(const int argc, const char **argv, clargs_t *clargs, int *exit_code)
{
    //region variables
    clargs->seps.elems_c = 0;
    clargs->seps.elems_v = NULL;
    int k = 0;
    //endregion

    /* if there is only name, functions and filename */
    if(argc >= 3 && strcmp(argv[1], "-d"))
    {
        set_add_item(&(clargs->seps), ' ', exit_code);
        CHECK_EXIT_CODE
    }

    /* if user entered -d flag and there are function names and filename in the commandline */
    else if(argc >= 5 && !strcmp(argv[1], "-d"))
    {
        while(argv[2][k])
        {
            /* checks if the user has not entered characters that will lead to undefined program behavior */
            if(argv[2][k] == 10 || argv[2][k] == 92 || argv[2][k] == 34)
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
#ifdef SEPSBUG
    printf("line %d in %s separators -> ", __LINE__, __FUNCTION__);
    for(int i = 0; i < clargs->seps.elems_c; i++ )
        printf("%c",clargs->seps.elems_v[i]);
    putchar(10);
#endif
}

void get_table(const int argc, const char *argv, table_t *table, clargs_t *clargs, int *exit_code)
{
    if((clargs->ptr = fopen(argv[argc - 1], "r+")) == NULL)
    {
        *exit_code = NO_SUCH_FILE_ERROR;
        return;
    }

    /* if file has been opened successfully */
    else
    {

    }
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
            "You've entered too few arguments",
            "There is no file with this name"
    };

    /* print an error message to stderr */
    fprintf(stderr, "Error %d: %s.\n", *exit_code, error_msg[*exit_code - 1]);
}

/**
 * Runs the program: initializes structures, then initializes commandline arguments, then processes the table
 * After every initializing checks for an exit_code to return it if the error has been occurred
 * Frees memory that was allocated while program was running.
 */
int run_program(const int argc, const char **argv)
{
    //region arguments
    int exit_code = 0;
    table_t  table;
    clargs_t clargs;
    //endregion

    /* initialize separators from commandline */
    init_separators(argc, argv, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* adds table to the structure */
    get_table(argc, argv, &table, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    clear_data(&table, &clargs);

    /* exit_code is -1 means table has been processed successfully */
    return (exit_code == -1) ? 0 : exit_code;
}

/* Prints documentation, Author */
void print_documentation()
{
    printf("Kiss an ugly turtle to make it cry");
}

int main(const int argc, const char **argv)
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
