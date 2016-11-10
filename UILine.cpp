#include <cstdlib>
#include <ctype.h>
#include "Database.h"
#include <iostream>
#include <readline/readline.h>
#include "Symbol.h"
#include "UILine.h"
#include <unistd.h>
#include <vector>

using namespace std;

static void report_results(const vector<Symbol> &vs) {
    cout << "cscope " << vs.size() << " lines\n";
    for (auto sym : vs) {
        const char *text = sym.context();
        cout << sym.path() << " " << sym.parent() << " " << sym.line() << " ";
        if (text) {
            const char *p = text;
            while (isspace(*p))
                p++;
            cout << (*p == '\0' ? "\n" : p);
        } else {
            cout << "\n";
        }
    }
}

int UILine::run(Database &db) {

    int ret = EXIT_SUCCESS;

    char *line;
    while ((line = readline(">> "))) {

        // Skip leading white space
        char *command = line;
        while (isspace(*command))
            command++;

        // Ignore blank lines
        if (strcmp(command, "") == 0) {
            free(line);
            continue;
        }

        switch (*command) {

            case '0': { // find symbol
                vector<Symbol> vs = db.find_symbols(command + 1);
                report_results(vs);
                break;
            }

            case '1': { // find definition
                vector<Symbol> vs = db.find_symbols(command + 1, ST_DEFINITION);
                report_results(vs);
                break;
            }

            case '2': // find callees
                break;

            case '3': // find callers
                break;

            case '7': // find file
                break;

            case '8': // find includers
                break;

            // Commands we don't support. Just pretend there were no results.
            case '4': // find text
            case '5': // change text
            case '6': // find pattern
            case '9': // find assignments
                cout << "cscope: 0 lines\n";
                break;

            /* Bail out on any unrecognised command, under the assumption Vim
             * would never send us a malformed command.
             */
            default:
                free(line);
                ret = EXIT_FAILURE;
                goto break2;

        }

        free(line);
    }

break2:

    return ret;
}
