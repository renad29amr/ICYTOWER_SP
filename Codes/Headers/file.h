#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <SFML/Graphics.hpp>
#include <algorithm>

using namespace std;
using namespace sf;

extern string userName;

struct User
{
    string user_name;
    int user_score;
};

const int MAX_USERS = 16;
User user_arr[MAX_USERS];
int user_count;
extern bool showHighScore, highScoreShownThisGame;
extern float highScoreTime;
extern View view;
extern Sprite highScore;
extern Sound sound_highScore;

void loadUserData()
{
    ifstream myFile("user_data.txt");
    string line;
    user_count = 0;

    while (getline(myFile, line) && user_count < MAX_USERS)
    {
        size_t lastSpace = line.rfind(' ');
        if (lastSpace != string::npos)
        {
            string name = line.substr(0, lastSpace);
            string scoreStr = line.substr(lastSpace + 1);

            try
            {
                int score = stoi(scoreStr);
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t") + 1);
                user_arr[user_count] = { name, score };
                user_count++;
            }

            catch (const exception& e)
            {
                continue;
            }
        }
    }
    myFile.close();
}

void saveUserData()
{
    sort(user_arr, user_arr + user_count, [](const User& a, const User& b)
    {
        return a.user_score > b.user_score;
    });

    ofstream myFile("user_data.txt");
    for (int i = 0; i < user_count; i++)
    {
        myFile << user_arr[i].user_name << " " << user_arr[i].user_score << endl;
    }
    myFile.close();
}

void updateOrAddUserScore(const string& name, int score)
{
    bool userExists = false;

    for (int i = 0; i < user_count; i++)
    {
        if (user_arr[i].user_name == name)
        {
            userExists = true;
            if (score > user_arr[i].user_score && !highScoreShownThisGame)
            {
                showHighScore = true;
                sound_highScore.play();
                user_arr[i].user_score = score; 
                highScoreTime = 0.0f;
                highScore.setPosition(view.getCenter().x + 800, view.getCenter().y - 80);
                highScore.setScale(0.5f, 0.5f);
                highScoreShownThisGame = true; 
            }

            else if (score > user_arr[i].user_score)
            {
                user_arr[i].user_score = score;
            }
            return;
        }
    }

    if (!userExists && user_count < MAX_USERS && !highScoreShownThisGame)
    {
        showHighScore = false;
        user_arr[user_count] = { name, score };
        user_count++;
        highScoreShownThisGame = true;
    }
}

string copyUserScore()
{
    stringstream s;

    for (int i = 0; i < user_count; i++)
    {
        s << i + 1 << ". " << user_arr[i].user_name << " - " << user_arr[i].user_score << "\n";
    }

    return s.str();
}
