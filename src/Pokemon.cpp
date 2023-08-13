#include "Pokemon.hpp"
#include <curl/curl.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <iostream>
// Constructor
Pokemon::Pokemon(std::string name, std::string image_url) : name(name), image_url(image_url) {}

// Getter for name
std::string Pokemon::getName() const
{
    return name;
}

// Setter for name
void Pokemon::setName(std::string name)
{
    this->name = name;
}

// Getter for image_url
std::string Pokemon::getImageUrl() const
{
    return image_url;
}

// Setter for image_url
void Pokemon::setImageUrl(std::string image_url)
{
    this->image_url = image_url;
}

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

void Pokemon::fetchFromApi(int id)
{
    // Prepare CURL
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        std::string url = "https://pokeapi.co/api/v2/pokemon/" + std::to_string(id);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);

        // Handle the results
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else
        {
            nlohmann::json jsonObject = nlohmann::json::parse(readBuffer);
            std::string name = jsonObject["name"];
            std::string image_url = jsonObject["sprites"]["front_default"];

            // Open SQLite database
            sqlite3 *db;
            int rc = sqlite3_open("pokemon.db", &db);
            if (rc)
            {
                fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                return;
            }

            // Create table if it doesn't exist
            char *errMsg;
            const char *createTableSQL = "CREATE TABLE IF NOT EXISTS Pokemon (Name TEXT UNIQUE, ImageUrl TEXT);";
            rc = sqlite3_exec(db, createTableSQL, 0, 0, &errMsg);

            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "Failed to create table: %s\n", errMsg);
                sqlite3_free(errMsg);
                sqlite3_close(db);
                return;
            }

            // Check if the Pokemon's name already exists in the database
            sqlite3_stmt *stmt;
            const char *checkSQL = "SELECT COUNT(*) FROM Pokemon WHERE Name = ?;";
            sqlite3_prepare_v2(db, checkSQL, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
            rc = sqlite3_step(stmt);
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt); // Always finalize the statement after use.

            if (count == 0)
            {
                // Insert
                const char *insertSQL = "INSERT INTO Pokemon (Name, ImageUrl) VALUES (?, ?);";
                rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
                sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, image_url.c_str(), -1, SQLITE_TRANSIENT);
            }
            else
            {
                // Update
                const char *updateSQL = "UPDATE Pokemon SET ImageUrl = ? WHERE Name = ?;";
                rc = sqlite3_prepare_v2(db, updateSQL, -1, &stmt, nullptr);
                sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 1, image_url.c_str(), -1, SQLITE_TRANSIENT);
            }

            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);
                return;
            }

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE)
            {
                fprintf(stderr, "Failed to execute SQL: %s\n", sqlite3_errmsg(db));
            }

            sqlite3_finalize(stmt);

            // Close the SQLite database
            sqlite3_close(db);
        }

        // Clean up CURL
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

std::vector<Pokemon> Pokemon::getPokemons()
{
    std::vector<Pokemon> pokemons; // Vector to hold all Pokemon objects

    // Open SQLite database
    sqlite3 *db;
    int rc = sqlite3_open("pokemon.db", &db);
    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return pokemons; // Return an empty vector if there's an error
    }

    // Get all the pokemons from the database
    sqlite3_stmt *stmt;
    const char *checkSQL = "SELECT Name, ImageUrl FROM Pokemon;";
    rc = sqlite3_prepare_v2(db, checkSQL, -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt); // Always finalize your prepared statements to avoid memory leaks.
        sqlite3_close(db);      // Close the database
        return pokemons;        // Return whatever Pokemons were added before the error
    }

    // Iterate over the result rows
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));     // First column: Name
        const char *imageUrl = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)); // Second column: ImageUrl

        if (name && imageUrl) // If neither name nor imageUrl is NULL
        {
            pokemons.emplace_back(name, imageUrl); // Construct a Pokemon object directly into the vector
        }
    }

    if (rc != SQLITE_DONE)
    {
        std::cerr << "SQL error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt); // Finalize the statement
    sqlite3_close(db);      // Close the database after all operations

    return pokemons; // Return the list of Pokemons
}