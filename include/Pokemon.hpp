#pragma once

#include <string>
#include <vector>
class Pokemon
{
private:
    std::string name;
    std::string image_url;

public:
    // Constructor
    Pokemon(std::string name, std::string image_url);

    // Getter for name
    std::string getName() const;

    // Setter for name
    void setName(std::string n);

    // Getter for image_url
    std::string getImageUrl() const;

    // Setter for image_url
    void setImageUrl(std::string image_url);

    static void fetchFromApi(int id);

    static std::vector<Pokemon> getPokemons();
};
