# Chaos Constructs
Um jogo inspirado em UT/DR:

Uma catástrofe aconteceu, liberando os construtos do caos no vilarejo de nara e resultando na captura da sua irmã. Agora nara terá de enfrentar os inimigos que aparecerem pela frente na esperança de reencontrar sua irmã perdida

# Instruções de Compilação
em windows (MinGW-W64) na pasta do projeto, execute build-MinGW-W64.bat. Ele usa o premake para baixar o raylib e gerar os Makefiles. Depois rode mingw32-make na pasta raiz. O executável fica em bin/Debug/ e deve ser executado a partir da pasta raiz do projeto, para encontrar a pasta resources.

execute build-VisualStudio2022.bat (ou build-VisualStudio2026.bat), abra a solução gerada e compile em Debug ou Release.

ja no Linux/macOs é so entrar em build/, rode ./premake5 gmake (ou ./premake5.osx gmake no macOS), volte para a raiz e rode make.
Requisitos: compilador C99 (GCC/MinGW ou MSVC). O raylib é baixado automaticamente pelo premake.
