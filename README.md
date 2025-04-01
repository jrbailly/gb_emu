## Emulateur gameboy
Cet émulateur est conçu pour reproduire le fonctionnement des consoles GameBoy originales.

Il ne gère actuellement que les modèles DMG (Dot Matrix Game) et les cartouches MBC1 (Memory Bank Controller 1).

Le projet est en cours de développement et vise à offrir une expérience authentique de jeu rétro.

### Répertoires
- docs : Contient la documentation du projet.
- inc : Fichiers d'en-tête (headers).
- src : Fichiers source en C++.
- tests : Inclut les tests unitaires.

### Dépendances
Pour compiler ce projet, assurez-vous d'avoir les outils et bibliothèques suivants installés sur votre système :
- gcc ou clang
- SDL3
- nlohmann_json
- Google Test
- CMake

### Compilation
    git clone https://github.com/jrbailly/gb_emu.git
    cd gb_emu
    mkdir build
    cd build
    cmake ..
    make

## Tests unitaires
Json de tests créés à partir du projet https://github.com/raddad772/jsmoo.git .

## État du projet
Ce projet est actuellement en cours de développement.
Fonctionnalités envisagées pour les futures versions :
- Support d'autres types de cartouches (MBC2, MBC3, etc.).
- Émulation de la GameBoy Color.
- Ajout d'une interface graphique.