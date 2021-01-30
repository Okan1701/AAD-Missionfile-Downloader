#include <iostream>
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>
#include <urlmon.h>
#include <string>
#include <vector>
#include "downloadStatusCallback.cpp"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "urlmon.lib")

using namespace std;

struct HttpRes {
    bool succes;
    char *ptrResponseBody;
    DWORD contentSize;

    virtual ~HttpRes() {
        delete[] ptrResponseBody;
    }
};

struct CommandArgs {
    bool hasApiToken = false;
    string apiToken = "";
};

/**
 * Simple method for converting char array to wchar array
 * @param text the const char array containing the text
 * @return wchar_t array with the converted text
 */
wchar_t* charToWChar(const char* text) {
    const size_t size = strlen(text) + 1;
    auto* wText = new wchar_t[size];
    mbstowcs(wText, text, size);
    return wText;
}

/**
 * Makes a HTTP request to fetch the mission list
 * The response body will be a raw JSON array
 * @param apiToken The API token used to authenticate the request
 * @return A pointer to a HttpRes struct containing details about the response
 */
HttpRes *getMissionList(string *apiToken) {
    auto *res = new HttpRes();
    res->succes = false;

    // Initialize WinHttp session and create a request handle
    HINTERNET hHttpSession = WinHttpOpen(
            L"WinHTTP AAD/Missiondownloader",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

    if (!hHttpSession) return res;

    HINTERNET hConnect = WinHttpConnect(hHttpSession, L"www.attackanddefend.co", INTERNET_DEFAULT_HTTPS_PORT, 0);

    if (!hConnect) return res;

    HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            nullptr,
            L"/api/v1/missionfiles",
            nullptr,
            WINHTTP_NO_REFERER,
            nullptr,
            WINHTTP_FLAG_SECURE);

    if (!hRequest) return res;

    wchar_t *token_wchar = charToWChar(apiToken->c_str());
    wstring tokenWStr(token_wchar);
    wstring authorization = L"Authorization: Bearer " + tokenWStr;
    if (!WinHttpAddRequestHeaders(hRequest, authorization.c_str(), -1L, 0)) return res;
    delete token_wchar;

    // Fetch the mission file list with the request handle
    WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    WinHttpReceiveResponse(hRequest, nullptr);

    // Check byte size of the response body data
    DWORD responseBodySize = 0;
    WinHttpQueryDataAvailable(hRequest, &responseBodySize);
    res->contentSize = responseBodySize;

    // Allocate enough memory to store response body in memory
    res->ptrResponseBody = new char[responseBodySize + 1];
    SecureZeroMemory(res->ptrResponseBody, responseBodySize + 1);

    // Read response data
    DWORD bytesDownloaded = 0;
    WinHttpReadData(hRequest, res->ptrResponseBody, responseBodySize, &bytesDownloaded);

    res->succes = true;
    return res;
}

/**
 * Parses the response body into an array of string filenames
 * Note that the response body is a simple JSON array
 * @param ptrResponseBody The pointer to a char array containing the response string
 */
void parseResponseString(char *ptrResponseBody, vector<string> *ptrFileNames) {
    string elementBuffer;
    char *ptrIterator = ptrResponseBody;
    vector<string> fileNames;

    // Iterate over the char array and extract the string elements
    // String elements will be placed into ptrFileNames
    while (*ptrIterator != '\0') {
        // If we encounter comma, we place current characters as element in array
        if (*ptrIterator == ',') {
            ptrFileNames->push_back(elementBuffer);
            elementBuffer = "";
        } else { // Place characters in a buffer, ignoring certain chars
            if (*ptrIterator != '[' && *ptrIterator != ']' && *ptrIterator != '"') {
                elementBuffer += *ptrIterator;
            }
        }

        // Advance the pointer to the next element
        ++ptrIterator;
    }

    // Move remaining characters in buffer to vector array
    if (!elementBuffer.empty()) ptrFileNames->push_back(elementBuffer);
}

/**
 * Prints the available mission files to console and asks user which file to download
 * @param ptrFileNames Pointer to a vector array containing the file names
 * @return The array index number of the selected file
 */
size_t getFileSelection(vector<string> *ptrFileNames) {
    cout << "Available mission files:" << endl;
    size_t curIndex = 1;
    size_t selectedIndex;
    size_t maxSize = ptrFileNames->size();
    for (auto i = ptrFileNames->begin(); i != ptrFileNames->end(); ++i) {
        printf("%zu. %s", curIndex, (*i).c_str());
        cout << endl;
        curIndex++;
    }
    printf("\nSelect file number to download [1-%zu]: ", maxSize);
    cin >> selectedIndex;

    while (selectedIndex < 1 || selectedIndex > maxSize) {
        cout << endl << "Invalid number specified!";
        printf("\nSelect file number to download [1-%zu]: ", maxSize);
        cin >> selectedIndex;
    }
    cout << endl;
    return selectedIndex - 1;
}

/**
 * Downloads the given filename from the A&D website
 * @param ptrFileName Pointer to string file name
 * @return HRESULT indicating if operation succeeded or not
 */
HRESULT downloadFile(string *ptrFileName) {
    // Get full path to %USERPROFILE%\AppData\Local folder
    wchar_t *appDataPathWChar;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appDataPathWChar);

    // Don't know how to work with wchar and normal char, so this is prob not good...
    wstring appDataPathWStr(appDataPathWChar);
    string appDataPathStr(appDataPathWStr.begin(), appDataPathWStr.end());

    string downloadUrl = "https://www.attackanddefend.co/api/v1/missionfiles/" + *ptrFileName;
    string savePath = appDataPathStr + R"(\Arma 3\MPMissionsCache\)" + *ptrFileName;
    printf("Saving file to %s\n", savePath.c_str());

    // Create a class instance that receives progress callbacks during download
    DownloadStatusCallback callback;

    // Start the main download
    HRESULT hDownRes = URLDownloadToFileA(
            nullptr,
            downloadUrl.c_str(),
            savePath.c_str(),
            0,
            &callback
    );
    cout << endl;
    return hDownRes;
}

/**
 * Check for the presence of any CLI args and parse it into a struct
 * @param ptrArgc Argument pointer count
 * @param ptrArgv Argument pointer value array
 * @param ptrArgs CommandArgs struct pointer to parse the raw data into
 */
void parseArgs(const int *ptrArgc, char *ptrArgv[], CommandArgs *ptrArgs) {
    if (*ptrArgc > 1) {
        ptrArgs->hasApiToken = true;
        ptrArgs->apiToken = ptrArgv[1];

    }
}

int main(int argc, char *argv[]) {
    // Parse the arguments into a struct
    CommandArgs parsedArgs;
    parseArgs(&argc, argv, &parsedArgs);

    // Title
    cout << "A&D ArmA 3 - Mission file download tool" << endl;

    // If token supplied, we present a selection screen
    if (parsedArgs.hasApiToken) {
        cout << "Fetching mission file list..." << endl;

        // Get http result containing raw response body
        // Since res is allocated in dynamic memoery, we will need to deallocate this later
        HttpRes *res = getMissionList(&parsedArgs.apiToken);

        // Display error and exit if failed
        if (!res->succes) {
            cout << endl << "ERROR occured while peforming WinHTTP request!";
            return GetLastError();
        }

        // A list that will contain all available file names
        vector<string> fileNames;

        // Parse the raw JSON string array and put it in the fileNames vector
        parseResponseString(res->ptrResponseBody, &fileNames);

        // Deallocate res since it is no longer needed
        delete res;

        cout << "  DONE!" << endl << "------------------" << endl;

        // Ask user which file needs to be downloaded
        size_t fileIndex = getFileSelection(&fileNames);
        printf("Downloading file: %s\n", fileNames[fileIndex].c_str());

        // Download the file
        downloadFile(&fileNames[fileIndex]);
    }
    else {
        string file0 = "aad.master.altis.pbo";
        string file1 = "aad.master.tanoa.pbo";

        downloadFile(&file0);
        downloadFile(&file1);
    }
    cout << endl << "Download finished!" << endl;
    MessageBeep(MB_ICONINFORMATION);
    system("pause");

    return S_OK;
}


