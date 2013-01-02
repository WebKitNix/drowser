#include "Browser.h"
#include "FatalError.h"
#include <iostream>

using namespace std;

int main(int argc, char** argv) try
{
    Browser browser;
    return browser.run();
} catch (const FatalError& e) {
    cerr << e.what() << endl;
    return 1;
}
