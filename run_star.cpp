#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>




// Sta³e
const int ROZMIAR_OKNA = 960; // Rozmiar okna gry
const float PROMIEN_GRACZA = 40.f; // Rozmiar gracza
const float PREDKOSC_OBROTU = 150.f; // Prêdkoœæ obrotu (stopnie na sekundê)
const float POCZATKOWY_PROMIEN = 1.f; // Pocz¹tkowy promieñ oktagonów
const float POCZATKOWY_MNOZNIK_ROZROSTU = 2.f; // Pocz¹tkowy mno¿nik rozrostu oktagonów
const float ZWIEKSZENIE_PREDKOSCI = 0.1f; // Zwiêkszenie mno¿nika rozrostu co sekundê
const int LICZBA_BOKOW = 8; // Liczba boków oktagonu
const float TOLERANCJA = 15; // Tolerancja na b³ad (hitbox)
const float ROZMIAR_WROGA = 20.f; // rozmiar wroga
const std::string NAZWA_PLIKU = "rekord.txt";

//Deklaracja zmiennych pod dŸwiêki 

sf::SoundBuffer shootBuffer;
sf::SoundBuffer enemyDeathBuffer;
sf::SoundBuffer playerDeathBuffer;

sf::Sound shootSound;
sf::Sound enemyDeathSound;
sf::Sound playerDeathSound;

//Deklaracja zmiennych pod muzykê 

sf::Music backgroundMusic;
sf::Music startbackgroundMusic;
sf::Music endbackgroundMusic;

bool trudno = false;
int liczbaZabitych = 0;

struct LiniaIKolo {
    std::vector<sf::Vertex> linia; // Linia (od gracza do œrodka)
    sf::CircleShape kolo;          // Kó³ko "zakotwiczone" do linii
    sf::Vector2f kierunek;         // Kierunek ruchu kó³ka
    float predkosc;                // Aktualna prêdkoœæ kó³ka
};


// Funkcja do odczytu rekordu z pliku
float odczytajRekord() {
    std::ifstream plik(NAZWA_PLIKU);
    if (plik.is_open()) {
        float rekord;
        plik >> rekord;
        plik.close();
        return rekord;
    }
    return 0.0f; // Domyœlnie 0, jeœli plik nie istnieje
}

// Funkcja do zapisu rekordu do pliku
void zapiszRekord(float rekord) {
    std::ofstream plik(NAZWA_PLIKU);
    if (plik.is_open()) {
        plik << rekord;
        plik.close();
    }
}




// Funkcja obracaj¹ca punkt wokó³ œrodka
void obrocPunkt(sf::Vector2f& punkt, const sf::Vector2f& srodek, float kat) {
    sf::Vector2f kierunek = punkt - srodek;
    float obroconyX = kierunek.x * std::cos(kat) - kierunek.y * std::sin(kat);
    float obroconyY = kierunek.x * std::sin(kat) + kierunek.y * std::cos(kat);
    punkt = srodek + sf::Vector2f(obroconyX, obroconyY);
}

// Funkcja obracaj¹ca liniê
void obrocLinie(sf::Vertex* linia, const sf::Vector2f& srodek, float kat) {
    for (int i = 0; i < 2; ++i) {
        obrocPunkt(linia[i].position, srodek, kat);
    }
}

// Funkcja tworz¹ca oktagon o zadanym promieniu i obrocie
std::vector<sf::Vector2f> utworzOktagon(const sf::Vector2f& srodek, float promien, float przesuniecieObrotu = 0.f) {
    std::vector<sf::Vector2f> wierzcholki;
    for (int i = 0; i < LICZBA_BOKOW; ++i) {
        float kat = 2 * 3.14159265f * i / LICZBA_BOKOW + przesuniecieObrotu;
        wierzcholki.emplace_back(srodek.x + promien * std::cos(kat),
            srodek.y + promien * std::sin(kat));
    }
    return wierzcholki;
}

// Funkcja generuj¹ca dziury w oktagonach
std::vector<int> generujDziury(int liczbaBokow) {
    std::vector<int> dziury;
    while (dziury.size() < 2) { // Dopóki liczba dziur jest mniejsza ni¿ 2
        dziury.clear(); // Wyczyœæ wektor dziur
        for (int i = 0; i < liczbaBokow; ++i) {
            if (std::rand() % 2 == 0) { // Losowo wybierz krawêdzie z dziurami
                dziury.push_back(i);
            }
        }
    }
    return dziury;
}

// Funkcja rysuj¹ca oktagon z dziurami
void rysujOktagonZDziurami(sf::RenderWindow& okno, const std::vector<sf::Vector2f>& wierzcholki, const std::vector<int>& dziury, sf::Color kolor) {
    for (size_t i = 0; i < wierzcholki.size(); ++i) {
        if (std::find(dziury.begin(), dziury.end(), i) != dziury.end()) {
            continue; // Pomiñ rysowanie krawêdzi z dziur¹
        }
        sf::Vertex linia[] = {
            sf::Vertex(wierzcholki[i], kolor),
            sf::Vertex(wierzcholki[(i + 1) % wierzcholki.size()], kolor)
        };
        okno.draw(linia, 2, sf::Lines);
    }
}

// Funkcja rysuj¹ca pe³ny oktagon
void rysujPelnyOktagon(sf::RenderWindow& okno, const std::vector<sf::Vector2f>& wierzcholki, sf::Color kolor) {
    for (size_t i = 0; i < wierzcholki.size(); ++i) {
        sf::Vertex linia[] = {
            sf::Vertex(wierzcholki[i], kolor),
            sf::Vertex(wierzcholki[(i + 1) % wierzcholki.size()], kolor)
        };
        okno.draw(linia, 2, sf::Lines);
    }
}

// Funkcja sprawdzaj¹ca kolizjê gracza z liniami
bool sprawdzKolizje(const sf::CircleShape& gracz, const std::vector<sf::Vector2f>& wierzcholki, const std::vector<int>& dziury) {
    sf::Vector2f pozycjaGracza = gracz.getPosition();

    for (size_t i = 0; i < wierzcholki.size(); ++i) {
        if (std::find(dziury.begin(), dziury.end(), i) != dziury.end()) {
            continue; // Pomiñ krawêdzie z dziur¹
        }

        sf::Vector2f start = wierzcholki[i];
        sf::Vector2f koniec = wierzcholki[(i + 1) % wierzcholki.size()];

        // Oblicz najkrótsz¹ odleg³oœæ od punktu do odcinka
        sf::Vector2f linia = koniec - start;
        sf::Vector2f doGracza = pozycjaGracza - start;

        float dlugoscLinii = std::sqrt(linia.x * linia.x + linia.y * linia.y);
        float t = std::max(0.f, std::min(1.f, (doGracza.x * linia.x + doGracza.y * linia.y) / (dlugoscLinii * dlugoscLinii)));

        sf::Vector2f najblizszyPunkt = start + t * linia;
        float dystans = std::sqrt(std::pow(pozycjaGracza.x - najblizszyPunkt.x, 2) + std::pow(pozycjaGracza.y - najblizszyPunkt.y, 2));

        if (dystans < PROMIEN_GRACZA - TOLERANCJA) {
            return true; // Kolizja wykryta
        }
    }

    return false;
}

struct Wrog {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float radius;
};

struct CelWroga {
    sf::Vector2f position;
    float radius;
};

std::vector<CelWroga> cele;

void spawnCelWroga(const sf::Vector2f& center, float promien) {
    CelWroga celWroga;
    float kat = static_cast<float>(rand()) / RAND_MAX * 2.f * 3.14f;

    // Oblicz wspó³rzêdne punktu na okrêgu
    float x = center.x + promien * std::cos(kat);
    float y = center.y + promien * std::sin(kat);
    celWroga.position = sf::Vector2f(x, y);
    cele.push_back(celWroga);
    celWroga.radius = 1.f;
}
void updateCele(float deltaTime) {
    for (auto it = cele.begin(); it != cele.end();) {
        it->radius *= 1.03f;
        if (it->radius > 50.f) {
            it = cele.erase(it);
        }
        else {
            ++it;
        }
    }
}


// Globalny schowek wrogow
std::vector<Wrog> wrogowie;

// Funkcja tworzaca wroga
void spawnWrog(const sf::Vector2f& position, const sf::Vector2f& cel) {
    Wrog wrog;
    wrog.position = position;
    sf::Vector2f direction = cel - position;
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length != 0.f) {
        wrog.velocity = direction / length * 10.f; // Pocz¹tkowa prêdkoœæ
    }
    else {
        wrog.velocity = sf::Vector2f(0.f, 0.f); // W przypadku d³ugoœci zero (gdyby cel == center)
    }
    wrog.radius = 1.f; // poczatkowy rozmiar
    wrogowie.push_back(wrog);
}

// Funckja aktualizujaca wrogów
void updateWrogowie(float deltaTime, const sf::Vector2f& center) {
    for (auto it = wrogowie.begin(); it != wrogowie.end();) {
        //obrocPunkt(it->position, center, kat);

        it->position += it->velocity * deltaTime;
        it->velocity *= 1.04f; // Powoli przyspieszaj
        it->radius *= 1.03f; // Rosnij po czasie

        // Usun wrogów jesli urosna zbyt mocno
        if (it->radius > 50.f) {
            it = wrogowie.erase(it);
        }
        else {
            ++it;
        }
    }
}

// Funkcja wysuj¹ca wrogów
void drawWrogowie(sf::RenderWindow& window, const sf::Texture& wrogTexture) {
    for (const auto& wrog : wrogowie) {
        sf::CircleShape shape(wrog.radius);
        shape.setOrigin(wrog.radius, wrog.radius);
        shape.setPosition(wrog.position);
        shape.setTexture(&wrogTexture);
        window.draw(shape);
    }
}
bool sprawdzKolizjeZWrogiem(const sf::CircleShape& gracz, const Wrog& wrog) {
    sf::Vector2f pozycjaGracza = gracz.getPosition();
    sf::Vector2f pozycjaWroga = wrog.position;

    // Oblicz odleg³oœæ miêdzy œrodkami gracza i wroga
    sf::Vector2f roznica = pozycjaGracza - pozycjaWroga;
    float odleglosc = std::sqrt(roznica.x * roznica.x + roznica.y * roznica.y);

    // SprawdŸ, czy odleg³oœæ jest mniejsza lub równa sumie promieni
    float sumaPromieni = gracz.getRadius() + wrog.radius;
    return odleglosc <= sumaPromieni;
}
void sprawdzKolizjeGraczaZWrogami(sf::CircleShape& gracz, std::vector<Wrog>& wrogowie) {
    for (auto it = wrogowie.begin(); it != wrogowie.end();) {
        if (sprawdzKolizjeZWrogiem(gracz, *it)) {
            // Reakcja na kolizjê (np. usuñ wroga i zmniejsz ¿ycie gracza)
            std::cout << "Kolizja z wrogiem!" << std::endl;
            it = wrogowie.erase(it); // Usuñ wroga z listy
        }
        else {
            ++it;
        }
    }
}
bool sprawdzKolizjeKulkaZWrogiem(const sf::CircleShape& kulka, const Wrog& wrog) {
    sf::Vector2f pozycjaKulka = kulka.getPosition();
    sf::Vector2f pozycjaWroga = wrog.position;

    // Oblicz odleg³oœæ miêdzy œrodkami kulki i wroga
    sf::Vector2f roznica = pozycjaKulka - pozycjaWroga;
    float odleglosc = std::sqrt(roznica.x * roznica.x + roznica.y * roznica.y);

    // SprawdŸ, czy odleg³oœæ jest mniejsza lub równa sumie promieni
    float sumaPromieni = kulka.getRadius() + wrog.radius;
    return odleglosc <= sumaPromieni;
}


int main() {

    // Inicjalizacja okna SFML
    sf::RenderWindow okno(sf::VideoMode(ROZMIAR_OKNA, ROZMIAR_OKNA), "RUNSTAR");
    okno.setFramerateLimit(60);

    // Definicja œrodka tunelu
    sf::Vector2f srodek(ROZMIAR_OKNA / 2.f, ROZMIAR_OKNA / 2.f);

    // Tworzenie osi wspó³rzêdnych
    sf::Vertex osX[] = {
     sf::Vertex(sf::Vector2f(-ROZMIAR_OKNA, srodek.y), sf::Color(7, 7, 62)),
     sf::Vertex(sf::Vector2f(ROZMIAR_OKNA * 2, srodek.y), sf::Color(7, 7, 62))
    };

    sf::Vertex osY[] = {
        sf::Vertex(sf::Vector2f(srodek.x, -ROZMIAR_OKNA), sf::Color(7, 7, 62)),
        sf::Vertex(sf::Vector2f(srodek.x, ROZMIAR_OKNA * 2), sf::Color(7, 7, 62))
    };

    sf::Vertex przekatna1[] = {
        sf::Vertex(sf::Vector2f(-ROZMIAR_OKNA, -ROZMIAR_OKNA), sf::Color(7, 7, 62)),
        sf::Vertex(sf::Vector2f(ROZMIAR_OKNA * 2, ROZMIAR_OKNA * 2), sf::Color(7, 7, 62))
    };

    sf::Vertex przekatna2[] = {
        sf::Vertex(sf::Vector2f(-ROZMIAR_OKNA, ROZMIAR_OKNA * 2), sf::Color(7, 7, 62)),
        sf::Vertex(sf::Vector2f(ROZMIAR_OKNA * 2, -ROZMIAR_OKNA), sf::Color(7, 7, 62))
    };




    // Tworzenie gracza
    sf::CircleShape gracz(PROMIEN_GRACZA);
    gracz.setOrigin(PROMIEN_GRACZA, PROMIEN_GRACZA);
    gracz.setPosition(srodek.x, ROZMIAR_OKNA - ROZMIAR_OKNA / 5.f);

    //Inicjalizacja dŸwiêków zawartoœciami plików

    if (!shootBuffer.loadFromFile("shoot.wav")) {
        std::cerr << "Nie mo¿na za³adowaæ shoot.wav\n";
    }
     if (!enemyDeathBuffer.loadFromFile("enemy_death.wav")) {
         std::cerr << "Nie mo¿na za³adowaæ enemy_death.wav\n";
     }
    if (!playerDeathBuffer.loadFromFile("ded.wav")) {
        std::cerr << "Nie mo¿na za³adowaæ ded.wav\n";
    }

    shootSound.setBuffer(shootBuffer);
    shootSound.setVolume(40);
    enemyDeathSound.setBuffer(enemyDeathBuffer);
    playerDeathSound.setBuffer(playerDeathBuffer);

    //inicjalizacja muzyki
    if (!backgroundMusic.openFromFile("background_music.mp3")) {
        std::cerr << "Nie mo¿na za³adowaæ background_music.mp3\n";
    }
    if (!startbackgroundMusic.openFromFile("startbackground_music.mp3")) {
        std::cerr << "Nie mo¿na za³adowaæ background_music.mp3\n";

    }
    if (!endbackgroundMusic.openFromFile("endbackground_music.mp3")) {
        std::cerr << "Nie mo¿na za³adowaæ background_music.mp3\n";
    }

    // Ladowanie tekstur
    sf::Texture graczTextureFront, graczTextureRight, graczTextureLeft, graczTextureExplosion1, graczTextureExplosion2, graczTextureExplosion3, graczTextureExplosion4, graczTextureExplosion5;
    if (!graczTextureFront.loadFromFile("STATEK_FRONT.png") ||
        !graczTextureLeft.loadFromFile("STATEK_LEFT.png") ||
        !graczTextureRight.loadFromFile("STATEK_RIGHT.png") ||
        !graczTextureExplosion1.loadFromFile("explosion1.png") ||
        !graczTextureExplosion2.loadFromFile("explosion2.png") ||
        !graczTextureExplosion3.loadFromFile("explosion3.png") ||
        !graczTextureExplosion4.loadFromFile("explosion4.png") ||
        !graczTextureExplosion5.loadFromFile("explosion5.png")) {
        std::cerr << "Nie udalo sie zaladowac jednego lub wiecej plikow." << std::endl;
        return -1;
    }

    gracz.setTexture(&graczTextureFront);

  /*  sf::Texture pocisk;
    if (!pocisk.loadFromFile("bullet.png")){
        std::cerr << "Nie udalo sie zaladowac jednego lub wiecej plikow." << std::endl;
        return -1;
    }*/


    sf::Texture backgroundTexture;
    if (ROZMIAR_OKNA == 960) {
        if (!backgroundTexture.loadFromFile("background960.png")) {
            std::cerr << "Nie udalo sie zaladowac pliku background.png" << std::endl;
            return -1;
        }
    }

    if (ROZMIAR_OKNA == 700) {
        if (!backgroundTexture.loadFromFile("background.png")) {
            std::cerr << "Nie udalo sie zaladowac pliku background.png" << std::endl;
            return -1;
        }
    }

    sf::Texture wrogTexture;
    if (!wrogTexture.loadFromFile("wrog.png")) {
        std::cerr << "Nie udalo sie zaladowac pliku wrog.png" << std::endl;
        return -1;
    }


    sf::Sprite backgroundSprite;
    backgroundSprite.setTexture(backgroundTexture);
    backgroundSprite.setScale(
        ROZMIAR_OKNA / backgroundTexture.getSize().x,
        ROZMIAR_OKNA / backgroundTexture.getSize().y
    );

    // Zmienna do kontrolowania ruchu
    enum class Direction { Front, Right, Left };
    Direction currentDirection = Direction::Front;

    // £adowanie czcionki
    sf::Font font;
    if (!font.loadFromFile("press.ttf")) {
        std::cerr << "Nie udalo sie zaladowac czcionki press.ttf" << std::endl;
        return -1;
    }

    // Timer
    sf::Clock zegarGry;

    sf::RectangleShape prostokatTimer(sf::Vector2f(300.f, 30.f));
    prostokatTimer.setFillColor(sf::Color(0, 0, 0, 150));
    prostokatTimer.setOutlineColor(sf::Color(117, 17, 137));
    prostokatTimer.setOutlineThickness(2.f);
    prostokatTimer.setPosition(10.f, 10.f);

    sf::Text tekstTimer;
    tekstTimer.setFont(font);
    tekstTimer.setCharacterSize(20);
    tekstTimer.setFillColor(sf::Color::White);
    tekstTimer.setPosition(20.f, 15.f);
    
    sf::Text tekstZabitych;
    tekstZabitych.setFont(font);
    tekstZabitych.setCharacterSize(20);
    tekstZabitych.setFillColor(sf::Color::White);
    tekstZabitych.setPosition(ROZMIAR_OKNA - 250.f, 15.f);

    sf::RectangleShape ramkaLicznika(sf::Vector2f(250.f, 30.f));
    ramkaLicznika.setFillColor(sf::Color(0, 0, 0, 150));
    ramkaLicznika.setOutlineColor(sf::Color(117, 17, 137));
    ramkaLicznika.setOutlineThickness(2.f);
    ramkaLicznika.setPosition(ROZMIAR_OKNA - 260.f, 10.f);

    // Inicjalizacja oktagonów
    std::vector<std::pair<std::vector<sf::Vector2f>, float>> oktagony;
    std::vector<std::pair<std::vector<sf::Vector2f>, float>> czerwoneOktagony;
    std::vector<std::vector<int>> dziuryWDanychOktagonach;
    float przesuniecieObrotu = 0.f; // Pocz¹tkowy obrót
    float aktualnyMnoznikRozrostu = POCZATKOWY_MNOZNIK_ROZROSTU;

    // Dodanie pierwszego oktagonu
    oktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN), POCZATKOWY_PROMIEN);
    czerwoneOktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN), POCZATKOWY_PROMIEN);
    dziuryWDanychOktagonach.push_back(generujDziury(LICZBA_BOKOW));

    std::vector<LiniaIKolo> linieIKolka; // Wektor przechowuj¹cy linie
    sf::Clock zegarLinii; // Zegar do tworzenia linii
    std::vector<sf::CircleShape> kolka; //pocisk

    sf::Clock zegar;
    sf::Clock zegarPrzyspieszenia;
    sf::Clock bulletClock;
    sf::Clock wrogClock;
    sf::Clock celClock;

    bool koniecGry = false;

    bool poczatekGry = true;

    while (okno.isOpen()) {
        sf::Event zdarzenie;
        while (okno.pollEvent(zdarzenie)) {
            if (zdarzenie.type == sf::Event::Closed)
                okno.close();


        }

        //Deklaracja napisów w menu
        sf::Text tekstStart;
        tekstStart.setFont(font);
        tekstStart.setString("START");
        tekstStart.setCharacterSize(50);
        tekstStart.setFillColor(sf::Color(255, 255, 255));
        tekstStart.setStyle(sf::Text::Bold);
        tekstStart.setPosition(ROZMIAR_OKNA / 2.f - tekstStart.getLocalBounds().width / 2.f,
            ROZMIAR_OKNA / 2.f - tekstStart.getLocalBounds().height / 5.f);

        sf::Text tekstRunStar;
        tekstRunStar.setFont(font);
        tekstRunStar.setString("RUN-STAR");
        tekstRunStar.setCharacterSize(80);
        tekstRunStar.setFillColor(sf::Color(255, 20, 0));
        tekstRunStar.setStyle(sf::Text::Bold);
        tekstRunStar.setPosition(ROZMIAR_OKNA / 2.f - tekstRunStar.getLocalBounds().width / 2.f,
            ROZMIAR_OKNA / 3.f - tekstRunStar.getLocalBounds().height / 0.7f);


        sf::Text tekstExit;
        tekstExit.setFont(font);
        tekstExit.setString("EXIT");
        tekstExit.setCharacterSize(50);
        tekstExit.setFillColor(sf::Color(255, 255, 255));
        tekstExit.setStyle(sf::Text::Bold);
        tekstExit.setPosition(ROZMIAR_OKNA / 2.f - tekstExit.getLocalBounds().width / 2.f,
            ROZMIAR_OKNA / 1.5f - tekstExit.getLocalBounds().height / 2.f);

        sf::Text tekstCredit;
        tekstCredit.setFont(font);
        tekstCredit.setString("M. Eryk, P. Krzysztof, \nP. Michal, S. Maciej");
        tekstCredit.setCharacterSize(20);
        tekstCredit.setFillColor(sf::Color(161, 52, 235));
        tekstCredit.setStyle(sf::Text::Bold);
        tekstCredit.setPosition(ROZMIAR_OKNA / 2.f - tekstExit.getLocalBounds().width / 0.7f,
            ROZMIAR_OKNA / 1.f - tekstCredit.getLocalBounds().height / 0.7f);



        // Aktualizacja tekstu licznika zabitych wrogów
        std::stringstream zabitychStream;
        zabitychStream << "Punkty: " << liczbaZabitych;
        tekstZabitych.setString(zabitychStream.str());

        float deltaCzas = zegar.restart().asSeconds();
        if (zegarPrzyspieszenia.getElapsedTime().asSeconds() > 1.f) {
            aktualnyMnoznikRozrostu += ZWIEKSZENIE_PREDKOSCI;
            zegarPrzyspieszenia.restart();
        }

        if (celClock.getElapsedTime().asSeconds() >= 2.f) { // Cel co 3 sekundy
            spawnCelWroga(srodek, 700.f); // Generowanie nowego celu
            celClock.restart(); // Restart zegara
        }

        // Spawn wrogów co 3 sekundy, jeœli s¹ cele
        if (wrogClock.getElapsedTime().asSeconds() >= 1.f) {
            if (!cele.empty()) {
                int losowyIndeks = rand() % cele.size(); // Wybierz losowy cel
                spawnWrog(srodek, cele[losowyIndeks].position); // Spawn wroga
            }
            wrogClock.restart(); // Reset zegara
        }

        if (zegarLinii.getElapsedTime().asSeconds() >= 0.2f) {
            // Tworzenie nowej linii
            std::vector<sf::Vertex> nowaLinia = {
                sf::Vertex(gracz.getPosition(), sf::Color::White),
                sf::Vertex(srodek, sf::Color::White)
                
              
            };
            if (trudno) {
               
                shootSound.play();
            }

            // Tworzenie nowego kó³ka
            sf::CircleShape noweKolo(PROMIEN_GRACZA / 6.f);
            noweKolo.setFillColor(sf::Color::White);
            noweKolo.setOrigin(noweKolo.getRadius(), noweKolo.getRadius());
            noweKolo.setPosition(gracz.getPosition());
            /*noweKolo.setTexture(&pocisk);*/
            

            // Oblicz kierunek ruchu kó³ka w stronê œrodka
            sf::Vector2f kierunek = srodek - gracz.getPosition();
            float dlugosc = std::sqrt(kierunek.x * kierunek.x + kierunek.y * kierunek.y);
            if (dlugosc > 0) {
                kierunek /= dlugosc; // Normalizacja
            }

            // Ustawienie pocz¹tkowej prêdkoœci (np. 200 pikseli/sekundê)
            float poczatkowaPredkosc = 200.f;

            // Dodaj liniê, kó³ko, kierunek i prêdkoœæ do wektora
            linieIKolka.push_back({ nowaLinia, noweKolo, kierunek, poczatkowaPredkosc });

            zegarLinii.restart();
        }
        for (auto& liniaIKolo : linieIKolka) {
            // Zwalnianie prêdkoœci o mno¿nik 0.97 co klatkê
            liniaIKolo.predkosc *= 0.98f;

            // Minimalna prêdkoœæ (np. 10 pikseli/sekundê)
            if (liniaIKolo.predkosc < 10.f) {
                liniaIKolo.predkosc = 10.f;
            }

            // Aktualizacja pozycji kó³ka wzd³u¿ kierunku
            liniaIKolo.kolo.move(liniaIKolo.kierunek * liniaIKolo.predkosc * deltaCzas);
        }
        for (auto& liniaIKolo : linieIKolka) {
            // Zmniejszanie rozmiaru kó³ka
            float mnoznikRozmiaru = 0.96f; // Mno¿nik zmniejszaj¹cy rozmiar co klatkê
            liniaIKolo.kolo.setScale(
                liniaIKolo.kolo.getScale().x * mnoznikRozmiaru,
                liniaIKolo.kolo.getScale().y * mnoznikRozmiaru
            );

            // Minimalny rozmiar kó³ka
            if (liniaIKolo.kolo.getScale().x < 0.1f) {
                liniaIKolo.kolo.setScale(0.1f, 0.1f);
            }

            // Aktualizacja pozycji kó³ka wzd³u¿ kierunku
            liniaIKolo.kolo.move(liniaIKolo.kierunek * liniaIKolo.predkosc * deltaCzas);
        }

        // Usuwanie kó³ek, które zbli¿y³y siê do œrodka
        linieIKolka.erase(std::remove_if(linieIKolka.begin(), linieIKolka.end(), [&](const LiniaIKolo& liniaIKolo) {
            // Oblicz odleg³oœæ od œrodka
            sf::Vector2f pozycjaKola = liniaIKolo.kolo.getPosition();
            float dystans = std::sqrt(std::pow(pozycjaKola.x - srodek.x, 2) + std::pow(pozycjaKola.y - srodek.y, 2));
            return dystans < 20.f; // Usuñ kó³ko, jeœli jest bli¿ej ni¿  pikseli od œrodka
            }), linieIKolka.end());

        for (auto itWrog = wrogowie.begin(); itWrog != wrogowie.end();) {
            bool wrogZniszczony = false;

            // Iterujemy przez wszystkie kulki
            for (const auto& liniaIKolo : linieIKolka) {
                const sf::CircleShape& kulka = liniaIKolo.kolo;

                // Sprawdzamy kolizjê miêdzy obecn¹ kulk¹ a wrogiem
                if (sprawdzKolizjeKulkaZWrogiem(kulka, *itWrog)) {
                    wrogZniszczony = true;
                    if(trudno){ enemyDeathSound.play(); }

                    break; // Nie trzeba sprawdzaæ dalej, wróg zosta³ trafiony
                }
            }

            // Usuñ wroga, jeœli zosta³ zniszczony
            if (wrogZniszczony) {
                liczbaZabitych++;
                itWrog = wrogowie.erase(itWrog);
            }
            else {
                ++itWrog; // PrzejdŸ do kolejnego wroga
            }
        }

        float katObrotu = 0.f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            katObrotu = PREDKOSC_OBROTU * deltaCzas;
            if (currentDirection != Direction::Right) {
                gracz.setTexture(&graczTextureRight);
                currentDirection = Direction::Right;
            }
            float rotationAngle = katObrotu * (3.14159265f / 180.f);
            for (auto& wrog : wrogowie) {
                float newX = wrog.velocity.x * std::cos(rotationAngle) - wrog.velocity.y * std::sin(rotationAngle);
                float newY = wrog.velocity.x * std::sin(rotationAngle) + wrog.velocity.y * std::cos(rotationAngle);
                wrog.velocity = sf::Vector2f(newX, newY);
            }
        }
        else {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                katObrotu = -PREDKOSC_OBROTU * deltaCzas;
                if (currentDirection != Direction::Left) {
                    gracz.setTexture(&graczTextureLeft);
                    currentDirection = Direction::Left;
                }
                float rotationAngle = katObrotu * (3.14159265f / 180.f);
                for (auto& wrog : wrogowie) {
                    float newX = wrog.velocity.x * std::cos(rotationAngle) - wrog.velocity.y * std::sin(rotationAngle);
                    float newY = wrog.velocity.x * std::sin(rotationAngle) + wrog.velocity.y * std::cos(rotationAngle);
                    wrog.velocity = sf::Vector2f(newX, newY);

                }
            }
            else {
                if (currentDirection != Direction::Front) {
                    gracz.setTexture(&graczTextureFront);
                    currentDirection = Direction::Front;
                    float rotationAngle = 0.f;
                    for (auto& wrog : wrogowie) {
                        float newX = wrog.velocity.x * std::cos(rotationAngle) - wrog.velocity.y * std::sin(rotationAngle);
                        float newY = wrog.velocity.x * std::sin(rotationAngle) + wrog.velocity.y * std::cos(rotationAngle);
                        wrog.velocity = sf::Vector2f(newX, newY);
                    }
                }
            }
        }
        if (katObrotu != 0.f) {
            float katRad = katObrotu * (3.14159265f / 180.f);
            przesuniecieObrotu += katRad;

            for (auto& liniaIKolo : linieIKolka) {
                // Obróæ liniê
                obrocLinie(liniaIKolo.linia.data(), srodek, katRad);

                // Obróæ kó³ko
                sf::Vector2f pozycjaKola = liniaIKolo.kolo.getPosition();
                obrocPunkt(pozycjaKola, srodek, katRad);
                liniaIKolo.kolo.setPosition(pozycjaKola);

                // Aktualizuj kierunek ruchu (od nowej pozycji kó³ka do œrodka)
                sf::Vector2f nowyKierunek = srodek - liniaIKolo.kolo.getPosition();
                float dlugosc = std::sqrt(nowyKierunek.x * nowyKierunek.x + nowyKierunek.y * nowyKierunek.y);
                if (dlugosc > 0) {
                    liniaIKolo.kierunek = nowyKierunek / dlugosc;
                }
            }


            obrocLinie(osX, srodek, katRad);
            obrocLinie(osY, srodek, katRad);
            obrocLinie(przekatna1, srodek, katRad);
            obrocLinie(przekatna2, srodek, katRad);
        }

        for (auto& kolo : kolka) {
            // Kierunek: od kó³ka do œrodka
            sf::Vector2f kierunek = srodek - kolo.getPosition();

            // Normalizacja kierunku
            float dlugosc = std::sqrt(kierunek.x * kierunek.x + kierunek.y * kierunek.y);
            if (dlugosc > 0) {
                kierunek /= dlugosc;
            }

            // Przesuñ kó³ko o sta³¹ prêdkoœæ (np. 200 pikseli na sekundê)
            float predkosc = 200.f;
            kolo.move(kierunek * predkosc * deltaCzas);
        }

        // Aktualizacja oktagonów
        for (size_t i = 0; i < oktagony.size(); ++i) {
            oktagony[i].second *= std::pow(aktualnyMnoznikRozrostu, deltaCzas);
            oktagony[i].first = utworzOktagon(srodek, oktagony[i].second, przesuniecieObrotu);
        }

        for (size_t i = 0; i < czerwoneOktagony.size(); ++i) {
            czerwoneOktagony[i].second *= std::pow(aktualnyMnoznikRozrostu, deltaCzas);
            czerwoneOktagony[i].first = utworzOktagon(srodek, czerwoneOktagony[i].second, przesuniecieObrotu);
        }



        if (oktagony.back().second > 12.f) {
            oktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
            czerwoneOktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
            dziuryWDanychOktagonach.push_back(generujDziury(LICZBA_BOKOW));

        }


        // Sprawdzanie kolizji
        for (size_t i = 0; i < czerwoneOktagony.size(); ++i) {
            if (sprawdzKolizje(gracz, czerwoneOktagony[i].first, dziuryWDanychOktagonach[i])) {
                koniecGry = true;
                playerDeathSound.play();
                break;
            }
        }

        if (trudno) {
            for (const auto& wrog : wrogowie) {
                if (sprawdzKolizjeZWrogiem(gracz, wrog)) {
                    koniecGry = true;
                    break;
                }
            }
        }

        // Funkcja aktualizacji wrogów
        updateWrogowie(deltaCzas, srodek);


        // Aktualizacja timera
        sf::Time czas = zegarGry.getElapsedTime();
        std::stringstream ss;
        ss << "Czas: " << std::fixed << std::setprecision(2) << czas.asSeconds() << " s";
        tekstTimer.setString(ss.str());

        // Rysowanie
        okno.clear();

        okno.draw(backgroundSprite);


        // Rysowanie osi i przek¹tnych
        okno.draw(osX, 2, sf::Lines);
        okno.draw(osY, 2, sf::Lines);
        okno.draw(przekatna1, 2, sf::Lines);
        okno.draw(przekatna2, 2, sf::Lines);

        for (const auto& liniaIKolo : linieIKolka) {
            // Pomijamy rysowanie linii:
            // okno.draw(liniaIKolo.linia.data(), 2, sf::Lines);

            // Rysujemy tylko kó³ka
            if (trudno) {
                okno.draw(liniaIKolo.kolo);
            }
        }

        // Rysowanie bia³ych oktagonów
        for (const auto& oktagon : oktagony) {
            rysujPelnyOktagon(okno, oktagon.first, sf::Color(207, 221, 255));
        }

        // Rysowanie czerwonych oktagonów z dziurami
        for (size_t i = 0; i < czerwoneOktagony.size(); ++i) {
            rysujOktagonZDziurami(okno, czerwoneOktagony[i].first, dziuryWDanychOktagonach[i], sf::Color::Red);
        }

        //rysowanie wrogów
        if (trudno) {
            drawWrogowie(okno, wrogTexture);
            okno.draw(ramkaLicznika);
            okno.draw(tekstZabitych);
        }
        // Rysowanie gracza
        okno.draw(gracz);

        // Rysowanie timera
        okno.draw(prostokatTimer);
        okno.draw(tekstTimer);


        okno.display();


        //Deklaracja tekstów w wyborze trudnoœci
        sf::Text tekstEasy;
        tekstEasy.setFont(font);
        tekstEasy.setString("EASY");
        tekstEasy.setCharacterSize(50);
        tekstEasy.setFillColor(sf::Color::Green);
        tekstEasy.setStyle(sf::Text::Bold);
        tekstEasy.setPosition(ROZMIAR_OKNA / 2.f - tekstEasy.getLocalBounds().width / 2.f,
            ROZMIAR_OKNA / 3.0f - tekstEasy.getLocalBounds().height / 2.f);

        sf::Text tekstHard;
        tekstHard.setFont(font);
        tekstHard.setString("HARD");
        tekstHard.setCharacterSize(50);
        tekstHard.setFillColor(sf::Color::Red);
        tekstHard.setStyle(sf::Text::Bold);
        tekstHard.setPosition(ROZMIAR_OKNA / 2.f - tekstHard.getLocalBounds().width / 2.f,
            ROZMIAR_OKNA / 1.5f - tekstHard.getLocalBounds().height / 2.f);

        bool wyborTrudnosci = false;

        if (poczatekGry) {
            //endbackgroundMusic.stop();
            startbackgroundMusic.setLoop(true);  // W³¹cz pêtlê
            startbackgroundMusic.setVolume(100); // Ustawienie g³oœnoœci (0-100)
            startbackgroundMusic.play();        // Rozpoczêcie odtwarzania
        }
        
        //if (koniecGry) {
        //    startbackgroundMusic.stop();
        //    endbackgroundMusic.setLoop(true);  // W³¹cz pêtlê
        //    endbackgroundMusic.setVolume(40); // Ustawienie g³oœnoœci (0-100)
        //    endbackgroundMusic.play();        // Rozpoczêcie odtwarzania

        //}


        //Wyœwietlanie menu
        while (poczatekGry) {

            
            sf::Event zdarzenie;
            while (okno.pollEvent(zdarzenie)) {
                if (zdarzenie.type == sf::Event::Closed)
                    okno.close();

                if (zdarzenie.type == sf::Event::MouseButtonPressed && zdarzenie.mouseButton.button == sf::Mouse::Left) {
                    sf::FloatRect boundingBox = tekstStart.getGlobalBounds();
                    if (boundingBox.contains(zdarzenie.mouseButton.x, zdarzenie.mouseButton.y)) {
                        poczatekGry = false;
                        wyborTrudnosci = true;


                    }
                }
            }

            if (zdarzenie.type == sf::Event::MouseButtonPressed && zdarzenie.mouseButton.button == sf::Mouse::Left) {
                sf::FloatRect boundingBox = tekstExit.getGlobalBounds();
                if (boundingBox.contains(zdarzenie.mouseButton.x, zdarzenie.mouseButton.y)) {
                    okno.close();
                    return 0;
                }
            }


            okno.clear(sf::Color::Black);
            okno.draw(tekstExit);
            okno.draw(tekstStart);
            okno.draw(tekstRunStar);
            okno.draw(tekstCredit);
            okno.display();
        }

        //Ekran wyboru trybu gry

        while (wyborTrudnosci) {
            sf::Event zdarzenie;
            while (okno.pollEvent(zdarzenie)) {
                if (zdarzenie.type == sf::Event::Closed) {
                    okno.close();
                }

                if (zdarzenie.type == sf::Event::MouseButtonPressed && zdarzenie.mouseButton.button == sf::Mouse::Left) {
                    sf::FloatRect boundingBox = tekstEasy.getGlobalBounds();
                    if (boundingBox.contains(zdarzenie.mouseButton.x, zdarzenie.mouseButton.y)) {

                        trudno = false;

                        //Reset zmiennych
                        wyborTrudnosci = false;
                        sf::Time czas = zegarGry.restart();
                        zegar.restart();
                        zegarPrzyspieszenia.restart();
                        przesuniecieObrotu = 0.f;
                        aktualnyMnoznikRozrostu = POCZATKOWY_MNOZNIK_ROZROSTU;
                        katObrotu = 0.f;
                        gracz.setTexture(&graczTextureFront);

                        //Reset Oktagonów
                        oktagony.clear();
                        czerwoneOktagony.clear();
                        dziuryWDanychOktagonach.clear();
                        oktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
                        czerwoneOktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
                        dziuryWDanychOktagonach.push_back(generujDziury(LICZBA_BOKOW));

                        //Reset osi
                        osX[0].position = sf::Vector2f(-ROZMIAR_OKNA, srodek.y);
                        osX[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, srodek.y);
                        osY[0].position = sf::Vector2f(srodek.x, -ROZMIAR_OKNA);
                        osY[1].position = sf::Vector2f(srodek.x, ROZMIAR_OKNA * 2);
                        przekatna1[0].position = sf::Vector2f(-ROZMIAR_OKNA, -ROZMIAR_OKNA);
                        przekatna1[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, ROZMIAR_OKNA * 2);
                        przekatna2[0].position = sf::Vector2f(-ROZMIAR_OKNA, ROZMIAR_OKNA * 2);
                        przekatna2[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, -ROZMIAR_OKNA);

                    }
                }

                if (zdarzenie.type == sf::Event::MouseButtonPressed && zdarzenie.mouseButton.button == sf::Mouse::Left) {
                    sf::FloatRect boundingBox = tekstHard.getGlobalBounds();
                    if (boundingBox.contains(zdarzenie.mouseButton.x, zdarzenie.mouseButton.y)) {

                        trudno = true;

                        //Reset zmiennych
                        wyborTrudnosci = false;
                        sf::Time czas = zegarGry.restart();
                        zegar.restart();
                        zegarPrzyspieszenia.restart();
                        przesuniecieObrotu = 0.f;
                        aktualnyMnoznikRozrostu = POCZATKOWY_MNOZNIK_ROZROSTU;
                        katObrotu = 0.f;
                        gracz.setTexture(&graczTextureFront);

                        //Reset Oktagonów
                        oktagony.clear();
                        czerwoneOktagony.clear();
                        dziuryWDanychOktagonach.clear();
                        oktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
                        czerwoneOktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
                        dziuryWDanychOktagonach.push_back(generujDziury(LICZBA_BOKOW));

                        //Reset osi
                        osX[0].position = sf::Vector2f(-ROZMIAR_OKNA, srodek.y);
                        osX[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, srodek.y);
                        osY[0].position = sf::Vector2f(srodek.x, -ROZMIAR_OKNA);
                        osY[1].position = sf::Vector2f(srodek.x, ROZMIAR_OKNA * 2);
                        przekatna1[0].position = sf::Vector2f(-ROZMIAR_OKNA, -ROZMIAR_OKNA);
                        przekatna1[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, ROZMIAR_OKNA * 2);
                        przekatna2[0].position = sf::Vector2f(-ROZMIAR_OKNA, ROZMIAR_OKNA * 2);
                        przekatna2[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, -ROZMIAR_OKNA);

                    }
                }


            }

            okno.clear(sf::Color::Black);
            okno.draw(tekstEasy);
            okno.draw(tekstHard);
            okno.display();

        }




        //Po kolizji
        if (koniecGry) {

            sf::Texture* explosionTextures[] = {
                      &graczTextureExplosion1,
                      &graczTextureExplosion2,
                      &graczTextureExplosion3,
                      &graczTextureExplosion4,
                      &graczTextureExplosion5
            };

            for (int i = 0; i < 5; ++i) {
                gracz.setTexture(explosionTextures[i]); // Ustaw kolejn¹ teksturê
                okno.draw(gracz); // Narysuj gracza z now¹ tekstur¹
                okno.display(); // Wyœwietl zmiany
                sf::sleep(sf::seconds(0.1f)); // Poczekaj 0.1 sekundy
            }
            bool nowyrekord = false;

            // Pobranie aktualnego czasu
            sf::Time czasKoncowy = zegarGry.getElapsedTime();

            // Odczytanie obecnego rekordu
            float rekord = odczytajRekord();

            // Sprawdzenie, czy nowy wynik jest lepszy
            if (czasKoncowy.asSeconds() > rekord) {
                rekord = czasKoncowy.asSeconds();
                zapiszRekord(rekord);
                nowyrekord = true;
            }
            sf::Text tekstZabiciNaKoniec;
            if (trudno) {
              
                tekstZabiciNaKoniec.setFont(font);
                tekstZabiciNaKoniec.setCharacterSize(25);
                tekstZabiciNaKoniec.setFillColor(sf::Color::White);
                std::stringstream zabiciNaKoniecStream;
                zabiciNaKoniecStream << "Zabici wrogowie: " << liczbaZabitych;
                tekstZabiciNaKoniec.setString(zabiciNaKoniecStream.str());
                tekstZabiciNaKoniec.setPosition(ROZMIAR_OKNA / 2.f - tekstZabiciNaKoniec.getLocalBounds().width / 2.f,
                    ROZMIAR_OKNA / 1.8f - tekstZabiciNaKoniec.getLocalBounds().height / 2.f);
               
            }
            liczbaZabitych = 0;
            // Wyœwietlenie napisu GAME OVER
            sf::Text tekstGameOver;
            tekstGameOver.setFont(font);
            tekstGameOver.setString("GAME OVER");
            tekstGameOver.setCharacterSize(50);
            tekstGameOver.setFillColor(sf::Color::Red);
            tekstGameOver.setStyle(sf::Text::Bold);
            tekstGameOver.setPosition(ROZMIAR_OKNA / 2.f - tekstGameOver.getLocalBounds().width / 2.f,
                ROZMIAR_OKNA / 4.0f - tekstGameOver.getLocalBounds().height / 2.f);

            sf::Text tekstCzas;
            tekstCzas.setFont(font);
            std::stringstream ss;
            ss << "Czas: " << std::fixed << std::setprecision(2) << czasKoncowy.asSeconds() << " s";
            tekstCzas.setString(ss.str());
            tekstCzas.setCharacterSize(30);
            tekstCzas.setFillColor(sf::Color::White);
            tekstCzas.setPosition(ROZMIAR_OKNA / 2.f - tekstCzas.getLocalBounds().width / 2.f,
                ROZMIAR_OKNA / 3.0f - tekstCzas.getLocalBounds().height / 2.f);

            sf::Text tekstRekord;
            tekstRekord.setFont(font);
            std::stringstream rekordStream;
            rekordStream << "Rekord: " << std::fixed << std::setprecision(2) << rekord << " s";
            tekstRekord.setString(rekordStream.str());
            tekstRekord.setCharacterSize(25);
            tekstRekord.setFillColor(sf::Color(117, 17, 137));
            tekstRekord.setPosition(ROZMIAR_OKNA / 2.f - tekstRekord.getLocalBounds().width / 2.f,
                ROZMIAR_OKNA / 2.5f - tekstRekord.getLocalBounds().height / 2.f);

            sf::Text tekstMenu;
            tekstMenu.setFont(font);
            tekstMenu.setString("Menu");
            tekstMenu.setCharacterSize(50);
            tekstMenu.setFillColor(sf::Color(117, 17, 137));
            tekstMenu.setStyle(sf::Text::Bold);
            tekstMenu.setPosition(ROZMIAR_OKNA / 2.f - tekstMenu.getLocalBounds().width / 2.f,
                ROZMIAR_OKNA / 1.5f - tekstMenu.getLocalBounds().height / 2.f);

            sf::Text tekstRestart;
            tekstRestart.setFont(font);
            tekstRestart.setString("Restart");
            tekstRestart.setCharacterSize(50);
            tekstRestart.setFillColor(sf::Color(117, 17, 137));
            tekstRestart.setStyle(sf::Text::Bold);
            tekstRestart.setPosition(ROZMIAR_OKNA / 2.f - tekstRestart.getLocalBounds().width / 2.f,
                ROZMIAR_OKNA / 1.3f - tekstRestart.getLocalBounds().height / 2.f);


            okno.clear(sf::Color::Black);
            okno.draw(tekstMenu);
            okno.draw(tekstRestart);
            okno.draw(tekstGameOver);
            okno.draw(tekstCzas);
            okno.draw(tekstRekord);
            okno.draw(tekstZabiciNaKoniec);

            if (nowyrekord) {
                sf::Text tekstNowyRekord;
                tekstNowyRekord.setFont(font);
                tekstNowyRekord.setString("NOWY REKORD!");
                tekstNowyRekord.setCharacterSize(30);
                tekstNowyRekord.setFillColor(sf::Color(237, 255, 41));
                tekstNowyRekord.setStyle(sf::Text::Bold);
                tekstNowyRekord.setPosition(ROZMIAR_OKNA / 2.f - tekstNowyRekord.getLocalBounds().width / 2.f,
                    ROZMIAR_OKNA / 2.0f - tekstNowyRekord.getLocalBounds().height / 2.f);
                okno.draw(tekstNowyRekord);
            }

            okno.display();

            while (koniecGry) {
                while (okno.pollEvent(zdarzenie)) {
                    if (zdarzenie.type == sf::Event::Closed)
                        okno.close();

                    if (zdarzenie.type == sf::Event::MouseButtonPressed && zdarzenie.mouseButton.button == sf::Mouse::Left) {
                        sf::FloatRect boundingBox = tekstMenu.getGlobalBounds();
                        if (boundingBox.contains(zdarzenie.mouseButton.x, zdarzenie.mouseButton.y)) {
                            koniecGry = false;
                            poczatekGry = true;
                        }
                    }

                    //Restartowanie po przegranej
                    if (zdarzenie.type == sf::Event::MouseButtonPressed && zdarzenie.mouseButton.button == sf::Mouse::Left) {
                        sf::FloatRect boundingBox = tekstRestart.getGlobalBounds();
                        if (boundingBox.contains(zdarzenie.mouseButton.x, zdarzenie.mouseButton.y)) {

                            //Reset zmiennych
                            koniecGry = false;
                            sf::Time czas = zegarGry.restart();
                            zegar.restart();
                            zegarPrzyspieszenia.restart();
                            przesuniecieObrotu = 0.f;
                            aktualnyMnoznikRozrostu = POCZATKOWY_MNOZNIK_ROZROSTU;
                            katObrotu = 0.f;
                            gracz.setTexture(&graczTextureFront);


                            //Reset Oktagonów
                            oktagony.clear();
                            czerwoneOktagony.clear();
                            dziuryWDanychOktagonach.clear();
                            oktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
                            czerwoneOktagony.emplace_back(utworzOktagon(srodek, POCZATKOWY_PROMIEN, przesuniecieObrotu), POCZATKOWY_PROMIEN);
                            dziuryWDanychOktagonach.push_back(generujDziury(LICZBA_BOKOW));

                            //Reset osi
                            osX[0].position = sf::Vector2f(-ROZMIAR_OKNA, srodek.y);
                            osX[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, srodek.y);
                            osY[0].position = sf::Vector2f(srodek.x, -ROZMIAR_OKNA);
                            osY[1].position = sf::Vector2f(srodek.x, ROZMIAR_OKNA * 2);
                            przekatna1[0].position = sf::Vector2f(-ROZMIAR_OKNA, -ROZMIAR_OKNA);
                            przekatna1[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, ROZMIAR_OKNA * 2);
                            przekatna2[0].position = sf::Vector2f(-ROZMIAR_OKNA, ROZMIAR_OKNA * 2);
                            przekatna2[1].position = sf::Vector2f(ROZMIAR_OKNA * 2, -ROZMIAR_OKNA);

                            //Reset wrogów i pocisków
                            wrogowie.clear();
                            wrogClock.restart();

                        }
                    }
                }


            }
        }

    }

    return 0;
}