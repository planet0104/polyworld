#include "interpreter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef PWHOME
#define PWHOME "."
#endif

#include "dom.h"
#include "parser.h"
#include "utils/misc.h"
#include "utils/Resources.h"

using namespace std;
using namespace proplib;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS ExpressionEvaluator
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Interpreter::ExpressionEvaluator::ExpressionEvaluator( Expression *expr )
: _expr( expr )
, _isEvaluating( false )
{
}

Interpreter::ExpressionEvaluator::~ExpressionEvaluator()
{
}

Expression *Interpreter::ExpressionEvaluator::getExpression()
{
	return _expr;
}

string Interpreter::ExpressionEvaluator::evaluate( Property *prop )
{
	if( _isEvaluating )
		prop->err( "Dependency cycle" );

	_isEvaluating = true;

	// ---
	// --- Generate Python Code
	// ---
	stringstream exprbuf;

	itfor( list<ExpressionElement *>, _expr->elements, it )
	{
		switch( (*it)->type )
		{
		case ExpressionElement::Misc:
			{
				MiscExpressionElement *element = dynamic_cast<MiscExpressionElement *>( *it );

				// Add leading whitespace if not first element.
				if( it != _expr->elements.begin() )
					exprbuf << element->token->getDecorationString();

				// Don't add trailing semicolon
				if( (element != _expr->elements.back()) || (element->token->type != Token::Semicolon) )
					exprbuf << element->token->text;
			}
			break;
		case ExpressionElement::Symbol:
			{
				SymbolExpressionElement *element = dynamic_cast<SymbolExpressionElement *>( *it );
				SymbolPath *symbolPath = element->symbolPath;
				
				// Add leading whitespace if not first element.
				if( it != _expr->elements.begin() )
					exprbuf << symbolPath->head->token->getDecorationString();
				
				Symbol sym;
				if( prop->findSymbol(symbolPath, sym) )
				{
					switch( sym.type )
					{
					case Symbol::EnumValue:
					case Symbol::Class:
						exprbuf << '"' << symbolPath->tail->getText() << '"';
						break;
					case Symbol::Property:
						{
							if( sym.prop->getType() != Node::Scalar )
								prop->err( string("Illegal reference to non-scalar ") + symbolPath->toString() + "." );

							if( sym.prop->getSubtype() == Node::Runtime )
								prop->err( string("Illegal reference to runtime property ") + symbolPath->toString() + ". Only dynamic expresssions may use runtime properties." );

							string value = (string)*sym.prop;
							if( sym.prop->isEnumValue(value) || sym.prop->isString() )
								exprbuf << '"' << value << '"';
							else
								exprbuf << value;
						}
						break;
					default:
						PANIC();
					}
				}
				else
				{
					// Hopefully a Python symbol.
					exprbuf << symbolPath->toString();
				}
			}
			break;
		default:
			PANIC();
		}
	}

	// ---
	// --- Execute Python Code
	// ---
	char result[1024 * 4];

	//cout << exprbuf.str() << endl;

	bool success = Interpreter::eval( exprbuf.str(), result, sizeof(result) );
	if( !success )
	{
		prop->err( string("[Python] ") + result );
	}

	_isEvaluating = false;

	return result;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS InterpreterProcess
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
#if defined(_WIN32) && !defined(__CYGWIN__)

static std::string pw_normalize_slashes( std::string path )
{
	for( size_t i = 0; i < path.size(); i++ )
		if( path[i] == '\\' )
			path[i] = '/';
	return path;
}

static bool pw_file_exists( const std::string &path )
{
	return GetFileAttributesA( path.c_str() ) != INVALID_FILE_ATTRIBUTES;
}

static std::string pw_python_executable()
{
	if( const char *prefix = getenv( "MSYSTEM_PREFIX" ) )
	{
		std::string exe = pw_normalize_slashes( prefix ) + "/bin/python3.exe";
		if( pw_file_exists( exe ) )
			return exe;
	}

	const char *fallbacks[] = {
		"C:/msys64/ucrt64/bin/python3.exe",
		"C:/msys64/mingw64/bin/python3.exe",
		NULL };
	for( int i = 0; fallbacks[i]; i++ )
	{
		if( pw_file_exists( fallbacks[i] ) )
			return fallbacks[i];
	}

	return "python3.exe";
}

static std::string pw_absolute_under_home( const std::string &path )
{
	if( path.empty() )
		return path;
	if( path.find( ':' ) != std::string::npos )
		return pw_normalize_slashes( path );
	if( !path.empty() && path[0] == '/' )
		return pw_normalize_slashes( path );

	std::string home = pw_normalize_slashes( PWHOME );
	if( !home.empty() && home[home.size() - 1] == '/' )
		return home + path;
	return home + "/" + path;
}

#endif

class InterpreterProcess {
#if defined(_WIN32) && !defined(__CYGWIN__)

    HANDLE hStdinWrite = INVALID_HANDLE_VALUE;
    HANDLE hStdoutRead = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION pi = {};

    ssize_t write(void const *buf, size_t n) {
        DWORD written = 0;
        if (!WriteFile(hStdinWrite, buf, (DWORD)n, &written, NULL))
            return -1;
        FlushFileBuffers(hStdinWrite);
        return (ssize_t)written;
    }

    ssize_t read(void *buf, size_t n) {
        char *out = (char *)buf;
        size_t total = 0;
        while( total < n )
        {
            DWORD nread = 0;
            if( !ReadFile( hStdoutRead, out + total, (DWORD)( n - total ), &nread, NULL ) )
                return total > 0 ? (ssize_t)total : -1;
            if( nread == 0 )
                return total > 0 ? (ssize_t)total : -1;
            total += nread;
        }
        return (ssize_t)total;
    }

    bool spawnPython( const std::string &python_exe, const std::string &script_path )
    {
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
        HANDLE hStdinRead = INVALID_HANDLE_VALUE;
        HANDLE hStdoutWrite = INVALID_HANDLE_VALUE;
        HANDLE hStderrSink = INVALID_HANDLE_VALUE;

        if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &sa, 0))
            return false;
        if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0)) {
            CloseHandle(hStdoutRead);
            CloseHandle(hStdoutWrite);
            return false;
        }

        SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);

        // GUI subsystem has no console; avoid invalid stderr breaking stdio setup.
        hStderrSink = CreateFileA( "NUL", GENERIC_WRITE, FILE_SHARE_WRITE,
                                   &sa, OPEN_EXISTING, 0, NULL );

        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = hStdinRead;
        si.hStdOutput = hStdoutWrite;
        si.hStdError = ( hStderrSink != INVALID_HANDLE_VALUE )
            ? hStderrSink
            : hStdoutWrite;

        std::string cmd = "\"" + python_exe + "\" -u \"" + script_path + "\"";
        std::vector<char> cmdbuf(cmd.begin(), cmd.end());
        cmdbuf.push_back('\0');

        SetEnvironmentVariableA("PYTHONUNBUFFERED", "1");
        SetEnvironmentVariableA("PYTHONIOENCODING", "utf-8");
        SetEnvironmentVariableA("PYTHONLEGACYWINDOWSSTDIO", "1");

        BOOL ok = CreateProcessA(
            python_exe.c_str(),
            cmdbuf.data(),
            NULL,
            NULL,
            TRUE,
            CREATE_NO_WINDOW,
            NULL,
            PWHOME,
            &si,
            &pi);

        CloseHandle(hStdinRead);
        CloseHandle(hStdoutWrite);
        if( hStderrSink != INVALID_HANDLE_VALUE )
            CloseHandle(hStderrSink);

        if (!ok) {
            CloseHandle(hStdinWrite);
            CloseHandle(hStdoutRead);
            hStdinWrite = INVALID_HANDLE_VALUE;
            hStdoutRead = INVALID_HANDLE_VALUE;
            return false;
        }

        CloseHandle(pi.hThread);
        return true;
    }

    void createPythonProcess() {
        std::string script_path = pw_absolute_under_home( Resources::getInterpreterScript() );
        REQUIRE( script_path != "" );
        REQUIRE( pw_file_exists( script_path ) );

        std::string python_exe = pw_python_executable();
        REQUIRE( pw_file_exists( python_exe ) );
        REQUIRE( spawnPython( python_exe, script_path ) );
    }

    void closePipes() {
        if (hStdinWrite != INVALID_HANDLE_VALUE) {
            CloseHandle(hStdinWrite);
            hStdinWrite = INVALID_HANDLE_VALUE;
        }
        if (hStdoutRead != INVALID_HANDLE_VALUE) {
            CloseHandle(hStdoutRead);
            hStdoutRead = INVALID_HANDLE_VALUE;
        }
        if (pi.hProcess) {
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess);
            pi.hProcess = NULL;
        }
    }

#else

    int const PIPE_READ = 0;
    int const PIPE_WRITE = 1;

    int stdinPipe[2];
    int stdoutPipe[2];

    ssize_t write(void const *buf, size_t n) {
        return ::write(stdinPipe[PIPE_WRITE], buf, n);
    }

    ssize_t read(void *buf, size_t n) {
        return ::read(stdoutPipe[PIPE_READ], buf, n);
    }

    void createPythonProcess() {
        REQUIRE( 0 == pipe(stdinPipe) );
        REQUIRE( 0 == pipe(stdoutPipe) );

        string script_path = Resources::getInterpreterScript();

        if(0 == fork()) {
            // child process

            // redirect stdin
            REQUIRE( -1 != dup2(stdinPipe[PIPE_READ], STDIN_FILENO) );
            // redirect stdout
            REQUIRE( -1 != dup2(stdoutPipe[PIPE_WRITE], STDOUT_FILENO) );

            setenv("PYTHONUNBUFFERED", "1", 1);

            // run child process image (python3 on Ubuntu 24.04; -u for pipe I/O)
            execlp("python3", "python3", "-u", script_path.c_str(), NULL);
            execlp("python", "python", "-u", script_path.c_str(), NULL);
            PANIC();
        }
    }

    void closePipes() {
        close(stdinPipe[PIPE_READ]);
        close(stdinPipe[PIPE_WRITE]);
        close(stdoutPipe[PIPE_READ]);
        close(stdoutPipe[PIPE_WRITE]);
    }

#endif

public:
    InterpreterProcess() {
        createPythonProcess();
    }

    ~InterpreterProcess() {
        write("exit\n", 5);
        closePipes();
    }

    bool eval(const std::string &expr,
              char *result, size_t result_size) {
        
        char writebuf[1024*8];
        sprintf(writebuf, "<expr>\n%s\n</expr>\n", expr.c_str());
        size_t writelen = strlen(writebuf);
        REQUIRE( ssize_t(writelen) == write(writebuf, writelen) );

        char success;
        REQUIRE( 1 == read(&success, 1) );
        REQUIRE( success == 'S' || success == 'F' );
        
        char readlen_str[11];
        REQUIRE( 10 == read(readlen_str, 10) );
        readlen_str[10] = '\0';
        size_t readlen = (size_t)atoi(readlen_str);
        
        REQUIRE( readlen < result_size );
        REQUIRE( ssize_t(readlen) == read(result, readlen) );
        result[readlen] = '\0';

        return success == 'S';
    }
};

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Interpreter
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

InterpreterProcess *Interpreter::process = nullptr;

void Interpreter::init()
{
	REQUIRE( !process );
    process = new InterpreterProcess();
}

void Interpreter::dispose()
{
	REQUIRE( process );
    delete process;
    process = nullptr;
}

bool Interpreter::eval( const std::string &expr,
						char *result, size_t result_size )
{
    return process->eval(expr, result, result_size);
}
