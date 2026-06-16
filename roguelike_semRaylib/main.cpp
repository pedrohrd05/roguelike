#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <conio.h>
#include <windows.h>

using namespace std;

const int MAP_W = 30;
const int MAP_H = 20;
const int MAX_FASES = 9;
const DWORD TEMPO_MOV_INIMIGO_MS = 450; // inimigos se movem automaticamente como no Bomberman

struct Vec2 {
    int x;
    int y;
};

enum Tela { MENU, CLASSES, JOGO, AJUDA, ITENS, PONTUACAO, LORE, LEVELUP, VITORIA, DERROTA, BATALHA, SAFE_ROOM, LOJA };
enum ClasseJogador { SEM_CLASSE, GUERREIRO, MAGO, LADINO, DEUS };
enum TileTipo { CHAO, PAREDE, PORTA, ARMADILHA, SAIDA };

struct Atributos {
    int forca = 2;
    int agilidade = 2;
    int vitalidade = 2;
    int armadura = 1;
};

struct Jogador {
    Vec2 pos{1, 1};
    int vida = 100;
    int vidaMax = 100;
    int nivel = 1;
    int xp = 0;
    int xpProx = 100;
    int pontosAtributo = 0;
    int chaves = 0;
    int pocao = 0;
    int score = 0;
    int movimentos = 0;
    int dinheiro = 0;
    int armaDur = 30;
    int armaDurMax = 30;
    bool escudo = false;
    bool imuneArmadilha = false;
    ClasseJogador classe = SEM_CLASSE;
    string nomeClasse = "Sem classe";
    string simbolo = "🙂";
    Atributos atr;
};

struct Inimigo {
    Vec2 pos{0, 0};
    int vida = 1;
    int vidaMax = 1;
    int dano = 1;
    bool persegue = false;
    bool boss = false;
};

struct Item {
    Vec2 pos{0, 0};
    int tipo = 0; // 0 pocao, 1 chave, 2 escudo, 3 XP
    bool ativo = true;
};

TileTipo mapa[MAP_H][MAP_W];
bool visto[MAP_H][MAP_W];
vector<Inimigo> inimigos;
vector<Item> itens;
Jogador player;
Tela tela = MENU;
int nivelAtual = 1;
bool bossDerrotado = false;
bool chaveLiberada = false;
int inimigoBatalha = -1;
string mensagem = "";
string textoBatalha = "";
Vec2 vendedor{15, 10};

void LimparTela() {
    // Em vez de usar system("cls") toda hora, o cursor volta para o topo.
    // Isso reduz bastante a piscada da tela no console.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD inicio = {0, 0};
    SetConsoleCursorPosition(hOut, inicio);
}

void LimparTelaCompleta() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD escritos;
    DWORD tamanho;
    COORD inicio = {0, 0};

    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        system("cls");
        return;
    }

    tamanho = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hOut, ' ', tamanho, inicio, &escritos);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, tamanho, inicio, &escritos);
    SetConsoleCursorPosition(hOut, inicio);
}

void VoltarMenuLimpo() {
    LimparTelaCompleta();
    mensagem = "";
    textoBatalha = "";
    tela = MENU;
}

void LimparFimLinha() {
    // Limpa do ponto atual ate o fim da linha sem imprimir varios espacos.
    // Isso evita quebrar a matriz quando o terminal usa emojis largos.
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        cout << "\n";
        return;
    }

    DWORD escritos;
    int faltam = csbi.dwSize.X - csbi.dwCursorPosition.X;
    if (faltam > 0) {
        FillConsoleOutputCharacter(hOut, ' ', faltam, csbi.dwCursorPosition, &escritos);
        FillConsoleOutputAttribute(hOut, csbi.wAttributes, faltam, csbi.dwCursorPosition, &escritos);
    }
}

void ConfigurarConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo = 0;
    GetConsoleMode(hOut, &modo);
    modo |= ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    SetConsoleMode(hOut, modo);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

int DistManhattan(Vec2 a, Vec2 b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

bool DentroMapa(int x, int y) {
    return x >= 0 && y >= 0 && x < MAP_W && y < MAP_H;
}

bool Bloqueia(TileTipo t) {
    return t == PAREDE || t == PORTA;
}

bool BordaMapa(int x, int y) {
    return x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1;
}

bool FaseComBoss(int nivel) {
    return nivel % 3 == 0;
}

string NomeBossAtual() {
    if (nivelAtual == 3) return "Riff Vale";
    if (nivelAtual == 6) return "Melodia Vale";
    if (nivelAtual == 9) return "Maestro Noctis";
    return "Guardiao da Cripta";
}

string ParentescoBossAtual() {
    if (nivelAtual == 3) return "seu irmao";
    if (nivelAtual == 6) return "sua mae";
    if (nivelAtual == 9) return "seu pai";
    return "um guardiao";
}

void SetMensagem(const string& txt) {
    mensagem = txt;
}

string NomeClasse(ClasseJogador c) {
    if (c == GUERREIRO) return "Guerreiro";
    if (c == MAGO) return "Mago";
    if (c == LADINO) return "Ladino";
    if (c == DEUS) return "Modo Deus";
    return "Sem classe";
}

void RevelarMapa() {
    int raio = 2;
    for (int y = player.pos.y - raio; y <= player.pos.y + raio; y++) {
        for (int x = player.pos.x - raio; x <= player.pos.x + raio; x++) {
            if (DentroMapa(x, y) && abs(x - player.pos.x) + abs(y - player.pos.y) <= raio + 1) {
                visto[y][x] = true;
            }
        }
    }
}

void AplicarClasse(ClasseJogador c) {
    player.classe = c;
    player.nomeClasse = NomeClasse(c);
    // Lore: o protagonista da aventura se chama Armon Vale.

    if (c == GUERREIRO) {
        player.simbolo = "⚔️";
        player.vidaMax = 110;
        player.vida = player.vidaMax;
        player.atr.forca = 3;
        player.atr.agilidade = 2;
        player.atr.vitalidade = 4;
        player.atr.armadura = 3;
        player.escudo = true;
        player.armaDurMax = 34;
    } else if (c == MAGO) {
        player.simbolo = "🔮";
        player.vidaMax = 65;
        player.vida = player.vidaMax;
        player.atr.forca = 5;
        player.atr.agilidade = 2;
        player.atr.vitalidade = 1;
        player.atr.armadura = 1;
        player.armaDurMax = 26;
    } else if (c == LADINO) {
        player.simbolo = "🗡️";
        player.vidaMax = 75;
        player.vida = player.vidaMax;
        player.atr.forca = 4;
        player.atr.agilidade = 4;
        player.atr.vitalidade = 2;
        player.atr.armadura = 1;
        player.imuneArmadilha = true;
        player.armaDurMax = 30;
    } else if (c == DEUS) {
        player.simbolo = "🖐️";
        player.vidaMax = 300;
        player.vida = player.vidaMax;
        player.atr.forca = 60;
        player.atr.agilidade = 25;
        player.atr.vitalidade = 50;
        player.atr.armadura = 40;
        player.escudo = true;
        player.imuneArmadilha = true;
        player.dinheiro = 500;
        player.pocao = 9;
        player.armaDurMax = 100;
    }
    player.armaDur = player.armaDurMax;
}

void GerarSafeRoom() {
    inimigos.clear();
    itens.clear();
    bossDerrotado = true;
    chaveLiberada = true;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            visto[y][x] = true;
            mapa[y][x] = (x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1) ? PAREDE : CHAO;
        }
    }

    for (int x = 4; x < MAP_W - 4; x++) {
        if (x != 14 && x != 15) mapa[5][x] = PAREDE;
        if (x != 14 && x != 15) mapa[15][x] = PAREDE;
    }

    vendedor = {15, 10};
    player.pos = {3, 10};
    tela = SAFE_ROOM;
    SetMensagem("SAFE ROOM: va ate o vendedor 💰 e aperte E.");
}

void GerarMapa(int nivel) {
    inimigos.clear();
    itens.clear();
    bossDerrotado = false;
    chaveLiberada = false;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            visto[y][x] = false;
            mapa[y][x] = (x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1) ? PAREDE : CHAO;
        }
    }

    for (int y = 2; y < MAP_H - 2; y += 4) {
        for (int x = 3; x < MAP_W - 3; x++) {
            if (x % 7 != 0) mapa[y][x] = PAREDE;
        }
    }
    for (int x = 5; x < MAP_W - 4; x += 8) {
        for (int y = 4; y < MAP_H - 3; y++) {
            if (y % 5 != 0) mapa[y][x] = PAREDE;
        }
    }

    // Blocos extras para o mapa parecer mais um labirinto.
    // Mantem o inicio e a saida livres para nao travar o jogador.
    int blocosLabirinto = 24 + nivel * 3;
    for (int i = 0; i < blocosLabirinto; i++) {
        int x = 2 + rand() % (MAP_W - 4);
        int y = 2 + rand() % (MAP_H - 4);

        bool areaInicial = (x <= 3 && y <= 3);
        bool areaSaida = (x >= MAP_W - 5 && y >= MAP_H - 5);

        if (mapa[y][x] == CHAO && !areaInicial && !areaSaida) {
            mapa[y][x] = PAREDE;
        }
    }

    for (int i = 0; i < 12 + nivel * 2; i++) {
        int x = 2 + rand() % (MAP_W - 4);
        int y = 2 + rand() % (MAP_H - 4);
        if (mapa[y][x] == CHAO && !(x < 4 && y < 4)) mapa[y][x] = ARMADILHA;
    }

    // Garante espaco livre no inicio.
    mapa[1][1] = CHAO;
    mapa[1][2] = CHAO;
    mapa[1][3] = CHAO;
    mapa[2][1] = CHAO;
    mapa[2][2] = CHAO;
    mapa[2][3] = CHAO;
    mapa[3][1] = CHAO;
    mapa[3][2] = CHAO;
    mapa[3][3] = CHAO;
    player.pos = {1, 1};

    // Garante espaco livre perto da saida.
    for (int y = MAP_H - 4; y < MAP_H - 1; y++) {
        for (int x = MAP_W - 5; x < MAP_W - 1; x++) {
            mapa[y][x] = CHAO;
        }
    }
    mapa[MAP_H - 2][MAP_W - 2] = SAIDA;
    mapa[MAP_H - 2][MAP_W - 3] = PORTA;

    for (int i = 0; i < 7 + nivel; i++) {
        int x, y;
        do {
            x = 2 + rand() % (MAP_W - 4);
            y = 2 + rand() % (MAP_H - 4);
        } while (mapa[y][x] != CHAO);
        int tiposSemChave[3] = {0, 2, 3};
        Item it;
        it.pos = {x, y};
        it.tipo = tiposSemChave[i % 3];
        itens.push_back(it);
    }

    for (int i = 0; i < 3 + nivel; i++) {
        int x, y;
        do {
            x = 3 + rand() % (MAP_W - 6);
            y = 3 + rand() % (MAP_H - 6);
        } while (mapa[y][x] != CHAO || DistManhattan({x, y}, player.pos) < 6);

        Inimigo e;
        e.pos = {x, y};
        e.vidaMax = 40 + nivel * 9;
        e.vida = e.vidaMax;
        e.dano = 7 + nivel * 2;
        e.persegue = true;
        e.boss = false;
        inimigos.push_back(e);
    }

    if (FaseComBoss(nivel)) {
        Inimigo b;
        b.pos = {MAP_W - 5, MAP_H - 5};
        b.vidaMax = 120 + nivel * 40;
        b.vida = b.vidaMax;
        b.dano = 12 + nivel * 3;
        b.persegue = true;
        b.boss = true;
        inimigos.push_back(b);
    }

    RevelarMapa();
}

void NovoJogo(ClasseJogador classeEscolhida) {
    player = Jogador();
    AplicarClasse(classeEscolhida);
    nivelAtual = 1;
    GerarMapa(nivelAtual);
    tela = JOGO;
    SetMensagem("Armon Vale entrou na Cripta Perdida como " + player.nomeClasse + ".");
}

void NovoJogoFaseFinalTeste() {
    player = Jogador();
    AplicarClasse(DEUS);
    nivelAtual = MAX_FASES;
    GerarMapa(nivelAtual);
    tela = JOGO;
    SetMensagem("Modo teste secreto: fase final liberada.");
}

bool ExisteInimigoEm(int x, int y) {
    for (const Inimigo &e : inimigos) {
        if (e.vida > 0 && e.pos.x == x && e.pos.y == y) return true;
    }
    return false;
}

bool CelulaLivreParaItem(int x, int y) {
    if (!DentroMapa(x, y)) return false;
    if (mapa[y][x] != CHAO) return false;
    if (player.pos.x == x && player.pos.y == y) return false;
    if (ExisteInimigoEm(x, y)) return false;
    for (const Item &it : itens) {
        if (it.ativo && it.pos.x == x && it.pos.y == y) return false;
    }
    return true;
}

int IndiceInimigoEm(int x, int y) {
    for (int i = 0; i < (int)inimigos.size(); i++) {
        if (inimigos[i].vida > 0 && inimigos[i].pos.x == x && inimigos[i].pos.y == y) return i;
    }
    return -1;
}

int ItemEm(int x, int y) {
    for (int i = 0; i < (int)itens.size(); i++) {
        if (itens[i].ativo && itens[i].pos.x == x && itens[i].pos.y == y) return i;
    }
    return -1;
}

bool PosicaoLivreParaInimigo(int x, int y) {
    if (!DentroMapa(x, y)) return false;
    if (Bloqueia(mapa[y][x])) return false;
    if (x == player.pos.x && y == player.pos.y) return false;
    if (ExisteInimigoEm(x, y)) return false;
    return true;
}

void ColetarItens() {
    int idx = ItemEm(player.pos.x, player.pos.y);
    if (idx == -1) return;

    Item &it = itens[idx];
    it.ativo = false;
    player.score += 60;

    if (it.tipo == 0) {
        player.pocao++;
        SetMensagem("Pocao 🧪 coletada! Use na batalha para gastar uma acao e curar.");
    } else if (it.tipo == 1) {
        player.chaves++;
        SetMensagem("Chave 🔑 coletada! A saida pode ser liberada.");
    } else if (it.tipo == 2) {
        player.escudo = true;
        SetMensagem("Escudo 🛡️ equipado! O proximo dano sera reduzido.");
    } else {
        player.xp += 20;
        SetMensagem("Cristal de XP ✨ coletado!");
    }
}

void AbrirPortas() {
    for (int y = player.pos.y - 1; y <= player.pos.y + 1; y++) {
        for (int x = player.pos.x - 1; x <= player.pos.x + 1; x++) {
            if (DentroMapa(x, y) && mapa[y][x] == PORTA && player.chaves > 0) {
                mapa[y][x] = CHAO;
                player.score += 120;
                SetMensagem("Porta 🚪 aberta. A chave sera usada na saida.");
            }
        }
    }
}

bool TodosInimigosMortos() {
    for (const Inimigo &e : inimigos) {
        if (e.vida > 0) return false;
    }
    return true;
}

void SpawnarChave() {
    int x = -1, y = -1;

    // A chave nasce perto do jogador, mas SOMENTE em celula vazia e ja revelada.
    // Assim ela nao cria "buracos" estranhos na fog of war e nao nasce dentro de parede.
    for (int raio = 1; raio <= 8 && x == -1; raio++) {
        for (int dy = -raio; dy <= raio && x == -1; dy++) {
            for (int dx = -raio; dx <= raio; dx++) {
                int tx = player.pos.x + dx;
                int ty = player.pos.y + dy;

                if (abs(dx) + abs(dy) > raio) continue;
                if (DentroMapa(tx, ty) && visto[ty][tx] && CelulaLivreParaItem(tx, ty)) {
                    x = tx;
                    y = ty;
                    break;
                }
            }
        }
    }

    // Se nao achar perto, procura qualquer celula ja revelada e vazia.
    if (x == -1) {
        for (int ty = 1; ty < MAP_H - 1 && x == -1; ty++) {
            for (int tx = 1; tx < MAP_W - 1; tx++) {
                if (visto[ty][tx] && CelulaLivreParaItem(tx, ty)) {
                    x = tx;
                    y = ty;
                    break;
                }
            }
        }
    }

    // Ultimo recurso: qualquer celula vazia do mapa.
    if (x == -1) {
        for (int ty = 1; ty < MAP_H - 1 && x == -1; ty++) {
            for (int tx = 1; tx < MAP_W - 1; tx++) {
                if (CelulaLivreParaItem(tx, ty)) {
                    x = tx;
                    y = ty;
                    break;
                }
            }
        }
    }

    if (x == -1) return;

    Item chave;
    chave.pos = {x, y};
    chave.tipo = 1;
    chave.ativo = true;
    itens.push_back(chave);

    // Garante que a propria chave esteja visivel sem revelar area extra.
    visto[y][x] = true;
}

void GanharXP(int valor) {
    player.xp += valor;
    while (player.xp >= player.xpProx) {
        player.xp -= player.xpProx;
        player.nivel++;
        player.pontosAtributo += 1;
        player.xpProx += 60 + player.nivel * 15;
        player.vidaMax += 6;
        player.vida += 12;
        if (player.vida > player.vidaMax) player.vida = player.vidaMax;
        tela = LEVELUP;
    }
}

void AplicarBonusBoss() {
    if (nivelAtual == 3) {
        player.vidaMax += 10;
        player.vida = min(player.vidaMax, player.vida + 10);
        SetMensagem("Bonus do boss: +10 de vida maxima.");
    } else if (nivelAtual == 6) {
        player.atr.armadura += 1;
        SetMensagem("Bonus do boss: +1 de armadura.");
    } else if (nivelAtual == 9) {
        player.atr.forca += 1;
        SetMensagem("Artefato final obtido: +1 de forca.");
    }
}

void EncerrarBatalha(bool venceu) {
    if (venceu && inimigoBatalha >= 0 && inimigoBatalha < (int)inimigos.size()) {
        Inimigo &e = inimigos[inimigoBatalha];
        e.vida = 0;
        int recompensa = e.boss ? 90 + nivelAtual * 10 : 10 + nivelAtual * 3;
        player.dinheiro += recompensa;
        player.score += e.boss ? 700 : 150;
        GanharXP(e.boss ? 90 + nivelAtual * 8 : 25 + nivelAtual * 3);

        if (e.boss) {
            if (nivelAtual == MAX_FASES) {
                tela = VITORIA;
                return;
            }

            bossDerrotado = true;
            AplicarBonusBoss();
            SetMensagem(NomeBossAtual() + " foi derrotado! 💀 SAFE ROOM liberada.");
        } else {
            SetMensagem("Inimigo derrotado! +" + to_string(recompensa) + " moedas.");
        }
    }
    inimigoBatalha = -1;
    textoBatalha = "";
    if (player.vida <= 0) tela = DERROTA;
    else if (tela != LEVELUP) tela = JOGO;
}

void TurnoInimigo() {
    if (inimigoBatalha < 0 || inimigoBatalha >= (int)inimigos.size()) return;
    Inimigo &e = inimigos[inimigoBatalha];
    if (e.vida <= 0) return;

    int dano = max(1, e.dano - player.atr.armadura * 2);
    int esquiva = player.atr.agilidade * 3;

    if ((rand() % 100) < esquiva) {
        textoBatalha += " Voce esquivou do contra-ataque!";
        return;
    }

    if (player.escudo) {
        dano /= 2;
        player.escudo = false;
        textoBatalha += " Seu escudo reduziu o dano.";
    }

    player.vida -= dano;
    textoBatalha += " Inimigo causou " + to_string(dano) + " de dano.";
    if (player.vida <= 0) tela = DERROTA;
}

void AcaoBatalha(int opcao) {
    if (inimigoBatalha < 0 || inimigoBatalha >= (int)inimigos.size()) {
        tela = JOGO;
        return;
    }

    Inimigo &e = inimigos[inimigoBatalha];

    if (opcao == 1) {
        int danoBase = 8;
        if (player.classe == GUERREIRO) danoBase = 9;
        else if (player.classe == MAGO) danoBase = 13;
        else if (player.classe == LADINO) danoBase = 10;

        int dano = danoBase + (player.atr.forca * 3) / 2 + player.nivel;
        if (player.armaDur <= player.armaDurMax / 3) dano = dano * 70 / 100;

        bool criticoArcano = false;
        if (player.classe == MAGO && (rand() % 100) < 20) {
            dano = dano * 3 / 2;
            criticoArcano = true;
        }

        int acerto = 70 + player.atr.agilidade * 4;
        if (player.armaDur > 0) player.armaDur--;

        if ((rand() % 100) < acerto) {
            e.vida -= dano;
            player.score += 25;
            textoBatalha = criticoArcano ? "Passiva do Mago ativou! Dano arcano: " : "Voce atacou e causou ";
            textoBatalha += to_string(dano) + " de dano.";
            if (e.vida <= 0) {
                EncerrarBatalha(true);
                return;
            }
        } else {
            textoBatalha = "Voce atacou, mas errou!";
        }
        TurnoInimigo();
    } else if (opcao == 2) {
        if (player.pocao > 0) {
            player.pocao--;
            int cura = 45 + player.atr.vitalidade * 5;
            player.vida = min(player.vidaMax, player.vida + cura);
            textoBatalha = "Voce usou uma pocao e recuperou vida.";
            TurnoInimigo();
        } else if (!player.escudo) {
            player.escudo = true;
            textoBatalha = "Sem pocoes. Voce preparou um escudo defensivo.";
            TurnoInimigo();
        } else {
            textoBatalha = "Voce nao tem itens de cura disponiveis.";
        }
    } else if (opcao == 3) {
        if (e.boss) {
            textoBatalha = "Nao da para fugir do chefe!";
            TurnoInimigo();
        } else {
            int chance = 45 + player.atr.agilidade * 5;
            if ((rand() % 100) < chance) {
                SetMensagem("Voce fugiu da batalha.");
                inimigoBatalha = -1;
                textoBatalha = "";
                tela = JOGO;
            } else {
                textoBatalha = "Falha ao fugir!";
                TurnoInimigo();
            }
        }
    }
}

void IniciarBatalha(int idx) {
    if (idx < 0 || idx >= (int)inimigos.size()) return;
    if (inimigos[idx].vida <= 0) return;
    inimigoBatalha = idx;
    tela = BATALHA;
    textoBatalha = inimigos[idx].boss ? (NomeBossAtual() + ", " + ParentescoBossAtual() + ", apareceu! Escolha sua acao.") : "Um inimigo apareceu! Escolha sua acao.";
}

Vec2 MelhorPassoPerseguicao(const Inimigo &e) {
    Vec2 opcoes[4] = {
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1}
    };

    // Embaralha um pouco para inimigos nao ficarem todos exatamente iguais.
    for (int i = 0; i < 4; i++) {
        int j = rand() % 4;
        Vec2 temp = opcoes[i];
        opcoes[i] = opcoes[j];
        opcoes[j] = temp;
    }

    Vec2 melhor = {0, 0};
    int melhorDist = DistManhattan(e.pos, player.pos);

    for (int i = 0; i < 4; i++) {
        int nx = e.pos.x + opcoes[i].x;
        int ny = e.pos.y + opcoes[i].y;

        if (!PosicaoLivreParaInimigo(nx, ny)) continue;

        int novaDist = DistManhattan({nx, ny}, player.pos);

        if (novaDist < melhorDist) {
            melhorDist = novaDist;
            melhor = opcoes[i];
        }
    }

    return melhor;
}

Vec2 PassoAleatorioInimigo(const Inimigo &e) {
    Vec2 opcoes[4] = {
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1}
    };

    for (int tent = 0; tent < 8; tent++) {
        int r = rand() % 4;
        int nx = e.pos.x + opcoes[r].x;
        int ny = e.pos.y + opcoes[r].y;

        if (PosicaoLivreParaInimigo(nx, ny)) {
            return opcoes[r];
        }
    }

    return {0, 0};
}

void AtualizarInimigos() {
    for (int i = 0; i < (int)inimigos.size(); i++) {
        Inimigo &e = inimigos[i];
        if (e.vida <= 0) continue;

        if (DistManhattan(e.pos, player.pos) <= 1) {
            IniciarBatalha(i);
            return;
        }

        Vec2 passo = {0, 0};
        int distancia = DistManhattan(e.pos, player.pos);

        // Boss persegue sempre. Inimigos normais perseguem quando o jogador esta perto.
        if (e.boss || distancia <= 12) {
            passo = MelhorPassoPerseguicao(e);
        }

        // Se nao conseguiu aproximar por causa de parede, tenta andar aleatorio.
        if (passo.x == 0 && passo.y == 0) {
            passo = PassoAleatorioInimigo(e);
        }

        int nx = e.pos.x + passo.x;
        int ny = e.pos.y + passo.y;

        if (PosicaoLivreParaInimigo(nx, ny)) {
            e.pos = {nx, ny};
        }

        if (DistManhattan(e.pos, player.pos) <= 1) {
            IniciarBatalha(i);
            return;
        }
    }
}

void TentarMoverJogador(int dx, int dy) {
    int nx = player.pos.x + dx;
    int ny = player.pos.y + dy;

    int idx = IndiceInimigoEm(nx, ny);
    if (idx != -1) {
        IniciarBatalha(idx);
        return;
    }

    if (!DentroMapa(nx, ny) || Bloqueia(mapa[ny][nx])) return;

    player.pos = {nx, ny};
    player.movimentos++;
    player.score = max(0, player.score - 1);

    if (mapa[ny][nx] == ARMADILHA) {
        if (player.imuneArmadilha) {
            mapa[ny][nx] = CHAO;
            SetMensagem("Ladino desarmou a armadilha 🕳️ sem dano.");
        } else {
            int dano = max(1, 12 + nivelAtual - player.atr.armadura);
            player.vida -= dano;
            mapa[ny][nx] = CHAO;
            player.score = max(0, player.score - 50);
            SetMensagem("Armadilha 🕳️ ativada! Sofreu " + to_string(dano) + " de dano.");
        }
    }

    if (mapa[ny][nx] == SAIDA) {
        if (player.chaves <= 0) {
            SetMensagem("Saida trancada. Derrote todos os inimigos e pegue a chave.");
        } else if (FaseComBoss(nivelAtual) && !bossDerrotado) {
            SetMensagem("Derrote o boss desta fase antes de sair!");
        } else if (FaseComBoss(nivelAtual) && bossDerrotado) {
            player.chaves--;
            GerarSafeRoom();
            return;
        } else if (nivelAtual < MAX_FASES) {
            player.chaves--;
            nivelAtual++;
            GerarMapa(nivelAtual);
            SetMensagem("Fase avancada! Inimigos ficaram mais fortes.");
            return;
        } else {
            player.chaves--;
            tela = VITORIA;
            return;
        }
    }

    ColetarItens();
    RevelarMapa();

    if (!chaveLiberada && TodosInimigosMortos()) {
        SpawnarChave();
        chaveLiberada = true;
        SetMensagem("Todos os inimigos foram derrotados! A chave 🔑 apareceu perto de voce.");
    }

    // Os inimigos agora se movem automaticamente pelo temporizador do loop principal.
    if (player.vida <= 0) tela = DERROTA;
}

void InteragirSafeRoom() {
    if (DistManhattan(player.pos, vendedor) <= 1) {
        tela = LOJA;
        SetMensagem("Vendedor: escolha o que deseja comprar.");
    } else {
        SetMensagem("Chegue perto do vendedor 💰 para interagir.");
    }
}

void ComprarLoja(int op) {
    if (op == 1) {
        if (player.dinheiro >= 25) { player.dinheiro -= 25; player.pocao++; SetMensagem("Comprou uma pocao 🧪."); }
        else SetMensagem("Dinheiro insuficiente para pocao.");
    } else if (op == 2) {
        if (player.dinheiro >= 40) { player.dinheiro -= 40; player.vida = player.vidaMax; SetMensagem("Vida totalmente curada."); }
        else SetMensagem("Dinheiro insuficiente para curar.");
    } else if (op == 3) {
        if (player.dinheiro >= 35) { player.dinheiro -= 35; player.escudo = true; SetMensagem("Escudo restaurado 🛡️."); }
        else SetMensagem("Dinheiro insuficiente para escudo.");
    } else if (op == 4) {
        if (player.dinheiro >= 30) { player.dinheiro -= 30; player.armaDur = player.armaDurMax; SetMensagem("Arma restaurada."); }
        else SetMensagem("Dinheiro insuficiente para restaurar arma.");
    } else if (op == 5) {
        if (player.dinheiro >= 60) { player.dinheiro -= 60; player.atr.forca++; player.armaDurMax += 4; player.armaDur = player.armaDurMax; SetMensagem("Melhoria comprada: +forca e durabilidade."); }
        else SetMensagem("Dinheiro insuficiente para melhoria.");
    }
}

void SairLojaEAvancar() {
    if (nivelAtual < MAX_FASES) {
        nivelAtual++;
        GerarMapa(nivelAtual);
        tela = JOGO;
        SetMensagem("Voce saiu da SAFE ROOM. Fase avancada!");
    } else {
        tela = VITORIA;
    }
}

string SimboloTile(int x, int y) {
    // Bordas do mapa ficam sempre visiveis, mesmo fora da fog of war.
    if (BordaMapa(x, y) && mapa[y][x] == PAREDE) return "🧱";

    // Fog of war: se ainda nao foi revelado, nao mostra nada.
    // Isso tambem esconde armadilhas, inimigos, boss, itens e chave.
    if (tela == JOGO && !visto[y][x]) return "  ";

    if (tela == SAFE_ROOM && x == vendedor.x && y == vendedor.y) return "💰";
    if (player.pos.x == x && player.pos.y == y) return player.simbolo;

    for (const Inimigo &e : inimigos) {
        if (e.vida > 0 && e.pos.x == x && e.pos.y == y) return e.boss ? "💀" : "👾";
    }

    if (mapa[y][x] == PAREDE) return "🧱";
    if (mapa[y][x] == PORTA) return "🚪";
    if (mapa[y][x] == ARMADILHA) return "🕳️";
    if (mapa[y][x] == SAIDA) return "🚩";

    int itemIdx = ItemEm(x, y);
    if (itemIdx != -1) {
        int tipo = itens[itemIdx].tipo;
        if (tipo == 0) return "🧪";
        if (tipo == 1) return "🔑";
        if (tipo == 2) return "🛡️";
        return "✨";
    }

    return "  ";
}

void DesenharMapaConsole() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            cout << SimboloTile(x, y);
        }
        LimparFimLinha();
        cout << "\n";
    }
}

void DesenharHUD() {
    cout << "\nFase: " << nivelAtual << "/" << MAX_FASES
         << " | Classe: " << player.nomeClasse
         << " | HP: " << player.vida << "/" << player.vidaMax
         << " | XP: " << player.xp << "/" << player.xpProx
         << " | $: " << player.dinheiro
         << " | Chaves: " << player.chaves
         << " | Pocoes: " << player.pocao
         << " | Arma: " << player.armaDur << "/" << player.armaDurMax;
    LimparFimLinha();
    cout << "\n";
    cout << "FOR " << player.atr.forca << " | AGI " << player.atr.agilidade << " | VIT " << player.atr.vitalidade
         << " | ARM " << player.atr.armadura << " | Score: " << player.score;
    LimparFimLinha();
    cout << "\n";
    cout << "\nControles: W/A/S/D mover | E interagir/abrir porta | ESPACO batalha perto | ESC menu";
    LimparFimLinha();
    cout << "\n";
    if (!mensagem.empty()) { cout << "\n>> " << mensagem; LimparFimLinha(); cout << "\n"; }
}

void DesenharJogo() {
    LimparTela();
    DesenharMapaConsole();
    DesenharHUD();
}

void DesenharSafeRoom() {
    LimparTela();
    DesenharMapaConsole();
    cout << "\nSAFE ROOM | Sem inimigos | Aproxime-se do vendedor 💰 e aperte E.\n";
    DesenharHUD();
}

void DesenharBatalha() {
    LimparTelaCompleta();
    if (inimigoBatalha < 0 || inimigoBatalha >= (int)inimigos.size()) return;
    Inimigo &e = inimigos[inimigoBatalha];

    cout << (e.boss ? "💀 BATALHA CONTRA O BOSS 💀\n" : "👾 BATALHA 👾\n");
    if (e.boss) cout << "Chefe: " << NomeBossAtual() << " (" << ParentescoBossAtual() << ")\n";
    cout << "\nInimigo HP: " << max(0, e.vida) << "/" << e.vidaMax << " | Dano: " << e.dano << "\n";
    cout << player.simbolo << " " << player.nomeClasse << " HP: " << max(0, player.vida) << "/" << player.vidaMax
         << " | Pocoes: " << player.pocao
         << " | Escudo: " << (player.escudo ? "SIM" : "NAO")
         << " | Arma: " << player.armaDur << "/" << player.armaDurMax << "\n";
    cout << "\n[1] Atacar\n[2] Curar / Usar item / Defender\n[3] Fugir\n";
    if (!textoBatalha.empty()) cout << "\n>> " << textoBatalha << "\n";
}

void DesenharLoja() {
    // Limpa a tela inteira para a loja aparecer sozinha, sem o mapa da safe room ao fundo.
    LimparTelaCompleta();
    cout << "💰 SAFE ROOM - VENDEDOR 💰\n\n";
    cout << "Moedas: " << player.dinheiro << " | HP: " << player.vida << "/" << player.vidaMax
         << " | Arma: " << player.armaDur << "/" << player.armaDurMax << " | Pocoes: " << player.pocao << "\n\n";
    cout << "[1] Comprar pocao 🧪 - 25 moedas\n";
    cout << "[2] Curar toda a vida ❤️ - 40 moedas\n";
    cout << "[3] Comprar/restaurar escudo 🛡️ - 35 moedas\n";
    cout << "[4] Restaurar durabilidade da arma ⚔️ - 30 moedas\n";
    cout << "[5] Melhorar arma e forca 🔥 - 60 moedas\n";
    cout << "[ENTER] Sair da SAFE ROOM e ir para proxima fase\n";
    if (!mensagem.empty()) cout << "\n>> " << mensagem << "\n";
}

void DesenharMenu() {
    LimparTelaCompleta();
    cout << "🏰 ROGUE DA CRIPTA PERDIDA 🏰\n\n";
    cout << "[ENTER] Escolher classe\n";
    cout << "[1] Como funciona\n";
    cout << "[2] Itens\n";
    cout << "[3] Pontuacao\n";
    cout << "[4] Lore\n";
    cout << "[ESC] Sair\n";
}

void DesenharClasses() {
    LimparTela();
    cout << "ESCOLHA SUA CLASSE\n\n";
    cout << "[1] ⚔️ GUERREIRO - Vida maior, armadura alta, comeca com escudo.\n";
    cout << "[2] 🔮 MAGO - Vida baixa, dano alto, passiva: critico arcano.\n";
    cout << "[3] 🗡️ LADINO - Agil, bom dano, imune a armadilhas.\n";
    cout << "\nESC volta ao menu.\n";
}

void DesenharTexto(const string& titulo, const vector<string>& linhas) {
    LimparTela();
    cout << titulo << "\n\n";
    for (const string &l : linhas) cout << l << "\n";
    cout << "\nPressione ESC para voltar.\n";
}

void DesenharLevelUp() {
    LimparTela();
    cout << "⭐ SUBIU DE NIVEL! ⭐\n\n";
    cout << "Pontos disponiveis: " << player.pontosAtributo << "\n\n";
    cout << "[1] Forca: aumenta dano\n";
    cout << "[2] Agilidade: aumenta acerto/esquiva\n";
    cout << "[3] Vitalidade: aumenta vida maxima\n";
    cout << "[4] Armadura: reduz dano recebido\n";
}

void DistribuirAtributo(int op) {
    if (player.pontosAtributo <= 0) return;
    if (op == 1) player.atr.forca++;
    if (op == 2) player.atr.agilidade++;
    if (op == 3) {
        player.atr.vitalidade++;
        player.vidaMax += 15;
        player.vida += 15;
        if (player.vida > player.vidaMax) player.vida = player.vidaMax;
    }
    if (op == 4) player.atr.armadura++;
    player.pontosAtributo--;
    if (player.pontosAtributo <= 0) tela = JOGO;
}

int main() {
    srand((unsigned)time(nullptr));
    ConfigurarConsole();

    bool rodando = true;
    while (rodando) {
        if (tela == MENU) {
            DesenharMenu();
            int c = _getch();
            if (c == 13) tela = CLASSES;
            else if (c == '1') tela = AJUDA;
            else if (c == '2') tela = ITENS;
            else if (c == '3') tela = PONTUACAO;
            else if (c == '4') tela = LORE;
            else if (c == 27) { LimparTelaCompleta(); rodando = false; }
        } else if (tela == CLASSES) {
            DesenharClasses();
            int c = _getch();
            if (c == '1') NovoJogo(GUERREIRO);
            else if (c == '2') NovoJogo(MAGO);
            else if (c == '3') NovoJogo(LADINO);
            else if (c == '9') NovoJogo(DEUS);
            else if (c == '7') NovoJogoFaseFinalTeste();
            else if (c == 27) VoltarMenuLimpo();
        } else if (tela == JOGO) {
            static DWORD ultimoMovimentoInimigo = GetTickCount();

            DesenharJogo();

            // Entrada sem travar o jogo. Assim os inimigos continuam andando mesmo se o jogador ficar parado.
            if (_kbhit()) {
                int c = _getch();
                mensagem = "";
                if (c == 'w' || c == 'W') TentarMoverJogador(0, -1);
                else if (c == 's' || c == 'S') TentarMoverJogador(0, 1);
                else if (c == 'a' || c == 'A') TentarMoverJogador(-1, 0);
                else if (c == 'd' || c == 'D') TentarMoverJogador(1, 0);
                else if (c == 'e' || c == 'E') AbrirPortas();
                else if (c == ' ') {
                    int idx = -1;
                    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                    for (auto &d : dirs) {
                        idx = IndiceInimigoEm(player.pos.x + d[0], player.pos.y + d[1]);
                        if (idx != -1) break;
                    }
                    if (idx != -1) IniciarBatalha(idx);
                    else SetMensagem("Nenhum inimigo perto para batalhar.");
                } else if (c == 27) VoltarMenuLimpo();
            }

            DWORD agora = GetTickCount();
            if (tela == JOGO && agora - ultimoMovimentoInimigo >= TEMPO_MOV_INIMIGO_MS) {
                AtualizarInimigos();
                ultimoMovimentoInimigo = agora;
                if (player.vida <= 0) tela = DERROTA;
            }

            // Sleep minimo para nao travar o processador e manter a limpeza/desenho do texto estavel.
            Sleep(1);
        } else if (tela == SAFE_ROOM) {
            DesenharSafeRoom();
            int c = _getch();
            mensagem = "";
            if (c == 'w' || c == 'W') TentarMoverJogador(0, -1);
            else if (c == 's' || c == 'S') TentarMoverJogador(0, 1);
            else if (c == 'a' || c == 'A') TentarMoverJogador(-1, 0);
            else if (c == 'd' || c == 'D') TentarMoverJogador(1, 0);
            else if (c == 'e' || c == 'E') InteragirSafeRoom();
            else if (c == 27) VoltarMenuLimpo();
        } else if (tela == BATALHA) {
            DesenharBatalha();
            int c = _getch();
            if (c == 27) VoltarMenuLimpo();
            else if (c == '1') AcaoBatalha(1);
            else if (c == '2') AcaoBatalha(2);
            else if (c == '3') AcaoBatalha(3);
        } else if (tela == LOJA) {
            DesenharLoja();
            int c = _getch();
            mensagem = "";
            if (c == '1') ComprarLoja(1);
            else if (c == '2') ComprarLoja(2);
            else if (c == '3') ComprarLoja(3);
            else if (c == '4') ComprarLoja(4);
            else if (c == '5') ComprarLoja(5);
            else if (c == 13) SairLojaEAvancar();
            else if (c == 27) VoltarMenuLimpo();
        } else if (tela == LEVELUP) {
            DesenharLevelUp();
            int c = _getch();
            if (c == '1') DistribuirAtributo(1);
            else if (c == '2') DistribuirAtributo(2);
            else if (c == '3') DistribuirAtributo(3);
            else if (c == '4') DistribuirAtributo(4);
            else if (c == 27) VoltarMenuLimpo();
        } else if (tela == AJUDA) {
            DesenharTexto("COMO FUNCIONA", {
                "Explore 9 fases, revele o mapa escuro e sobreviva ate o boss final.",
                "Movimento em grade igual Bomberman: W/A/S/D anda um bloco por vez.",
                "Encoste ou aperte ESPACO perto de inimigos para iniciar batalha por turno.",
                "Mate todos os inimigos para liberar a chave.",
                "A cada 3 fases existe um boss; depois dele abre uma SAFE ROOM."
            });
            if (_getch() == 27) VoltarMenuLimpo();
        } else if (tela == ITENS) {
            DesenharTexto("ITENS", {
                "🧪 Pocao: cura na batalha e gasta uma rodada.",
                "🔑 Chave: aparece apos derrotar todos os inimigos.",
                "🛡️ Escudo: reduz o proximo dano recebido.",
                "✨ Cristal: concede experiencia.",
                "🕳️ Armadilha: causa dano, exceto no Ladino."
            });
            if (_getch() == 27) VoltarMenuLimpo();
        } else if (tela == LORE) {
            DesenharTexto("LORE - A CRIPTA PERDIDA", {
                "Armon Vale nasceu da uniao entre uma humana e um demonio da Cripta Perdida.",
                "Criado longe das trevas, ele viveu sem conhecer sua verdadeira origem.",
                "Ao descobrir que sua familia dominava a cripta e espalhava sofrimento pelo reino,",
                "Armon decidiu descer ate as profundezas e enfrentar o proprio sangue.",
                "",
                "Fase 3 - Riff Vale, seu irmao.",
                "Fase 6 - Melodia Vale, sua mae.",
                "Fase 9 - Maestro Noctis, seu pai e Senhor da Cripta.",
                "",
                "Somente derrotando sua linhagem Armon podera quebrar a maldicao",
                "ou assumir para sempre o trono da Cripta Perdida."
            });
            if (_getch() == 27) VoltarMenuLimpo();
        } else if (tela == PONTUACAO) {
            DesenharTexto("PONTUACAO", {
                "+ pontos por itens, portas, inimigos, bosses e progresso.",
                "+ dinheiro ao derrotar inimigos; bosses dao mais moedas.",
                "- pontos por excesso de movimentos e armadilhas.",
                "O HUD mostra vida, classe, dinheiro, arma, XP e atributos."
            });
            if (_getch() == 27) VoltarMenuLimpo();
        } else if (tela == VITORIA) {
            LimparTelaCompleta();
            cout << "🏆 PARABENS! 🏆\n\n";
            cout << "Armon Vale derrotou Maestro Noctis, seu pai e Senhor da Cripta.\n";
            cout << "Obrigado por jogar este projeto.\n";
            cout << "Fim da aventura.\n\n";
            cout << "Pontuacao final: " << player.score << "\n\n";
            cout << "ENTER: jogar novamente | ESC: menu\n";
            int c = _getch();
            if (c == 13) tela = CLASSES;
            else if (c == 27) VoltarMenuLimpo();
        } else if (tela == DERROTA) {
            LimparTela();
            cout << "💀 DERROTA 💀\n";
            cout << "Sua vida chegou a zero.\n";
            cout << "Pontuacao final: " << player.score << "\n\n";
            cout << "ENTER: tentar novamente | ESC: menu\n";
            int c = _getch();
            if (c == 13) tela = CLASSES;
            else if (c == 27) VoltarMenuLimpo();
        }
    }

    return 0;
}
