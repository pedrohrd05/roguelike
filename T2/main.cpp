

#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

const int TILE = 32;
const int MAP_W = 30;
const int MAP_H = 20;
const int SCREEN_W = MAP_W * TILE;
const int SCREEN_H = MAP_H * TILE + 90;
const int MAX_FASES = 9;

enum Tela { MENU, CLASSES, JOGO, AJUDA, ITENS, PONTUACAO, LEVELUP, VITORIA, DERROTA, BATALHA, SAFE_ROOM, LOJA };
enum ClasseJogador { SEM_CLASSE, GUERREIRO, MAGO, LADINO };
enum TileTipo { CHAO, PAREDE, PORTA, ARMADILHA, SAIDA, NPC_TILE };

struct Atributos {
    int forca = 2;
    int agilidade = 2;
    int vitalidade = 2;
    int armadura = 1;
};

struct Jogador {
    Vector2 pos;
    float raio = 12;
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
    float ataqueCooldown = 0;
    ClasseJogador classe = SEM_CLASSE;
    string nomeClasse = "Sem classe";
    Color cor = RAYWHITE;
    Atributos atr;
};

struct Inimigo {
    Vector2 pos;
    int vida;
    int vidaMax;
    int dano;
    float vel;
    bool persegue;
    bool boss;
    float trocaDir = 0;
    Vector2 dir;
};

struct Item {
    Vector2 pos;
    int tipo; // 0 pocao, 1 chave, 2 escudo, 3 poder
    bool ativo = true;
};

struct Npc {
    Vector2 pos;
    bool falou = false;
    bool abriuPassagem = false;
};

TileTipo mapa[MAP_H][MAP_W];
bool visto[MAP_H][MAP_W];
vector<Inimigo> inimigos;
vector<Item> itens;
Npc npc;
Jogador player;
Tela tela = MENU;
int nivelAtual = 1;
bool bossDerrotado = false;
string mensagem = "";
float msgTimer = 0;
int inimigoBatalha = -1;
string textoBatalha = "";
bool chaveLiberada = false;

float Dist(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx*dx + dy*dy);
}

bool DentroMapa(int x, int y) {
    return x >= 0 && y >= 0 && x < MAP_W && y < MAP_H;
}

bool Bloqueia(TileTipo t) {
    return t == PAREDE || t == PORTA || t == NPC_TILE;
}

bool PosicaoBloqueada(Vector2 p, float raio) {
    int minX = (int)((p.x - raio) / TILE);
    int maxX = (int)((p.x + raio) / TILE);
    int minY = (int)((p.y - raio) / TILE);
    int maxY = (int)((p.y + raio) / TILE);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            if (!DentroMapa(x, y)) return true;
            if (Bloqueia(mapa[y][x])) {
                Rectangle r = {(float)x*TILE, (float)y*TILE, (float)TILE, (float)TILE};
                if (CheckCollisionCircleRec(p, raio, r)) return true;
            }
        }
    }
    return false;
}

void SetMensagem(const string& txt) {
    mensagem = txt;
    msgTimer = 3.0f;
}

Vector2 CentroTile(int x, int y) {
    return {(float)x*TILE + TILE/2.0f, (float)y*TILE + TILE/2.0f};
}

void RevelarMapa() {
    int px = (int)(player.pos.x / TILE);
    int py = (int)(player.pos.y / TILE);
    int raio = 4;
    for (int y = py - raio; y <= py + raio; y++) {
        for (int x = px - raio; x <= px + raio; x++) {
            if (DentroMapa(x, y) && abs(x - px) + abs(y - py) <= raio + 1) {
                visto[y][x] = true;
            }
        }
    }
}


Color CorClasse(ClasseJogador c) {
    if (c == GUERREIRO) return BLUE;
    if (c == MAGO) return VIOLET;
    if (c == LADINO) return LIME;
    return RAYWHITE;
}

string NomeClasse(ClasseJogador c) {
    if (c == GUERREIRO) return "Guerreiro";
    if (c == MAGO) return "Mago";
    if (c == LADINO) return "Ladino";
    return "Sem classe";
}

bool FaseComBoss(int nivel) {
    return nivel % 3 == 0;
}

bool EstaNaSafeRoom() {
    return tela == SAFE_ROOM;
}

void GerarSafeRoom() {
    inimigos.clear();
    itens.clear();
    bossDerrotado = true;
    chaveLiberada = true;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            visto[y][x] = true;
            if (x == 0 || y == 0 || x == MAP_W-1 || y == MAP_H-1)
                mapa[y][x] = PAREDE;
            else
                mapa[y][x] = CHAO;
        }
    }

    // Paredes decorativas para dar cara de sala segura.
    for (int x = 4; x < MAP_W - 4; x++) {
        if (x != 14 && x != 15) mapa[5][x] = PAREDE;
        if (x != 14 && x != 15) mapa[15][x] = PAREDE;
    }

    // NPC vendedor no centro da safe room.
    npc.pos = CentroTile(15, 10);
    mapa[10][15] = NPC_TILE;
    npc.falou = false;
    npc.abriuPassagem = false;

    // Player nasce na entrada da sala.
    player.pos = CentroTile(3, 10);

    SetMensagem("SAFE ROOM: aproxime-se do NPC vendedor e aperte E.");
}


void AplicarClasse(ClasseJogador c) {
    player.classe = c;
    player.nomeClasse = NomeClasse(c);
    player.cor = CorClasse(c);

    if (c == GUERREIRO) {
        player.vidaMax = 110;
        player.vida = player.vidaMax;
        player.atr.forca = 3;
        player.atr.agilidade = 2;
        player.atr.vitalidade = 4;
        player.atr.armadura = 3;
        player.escudo = true;
        player.armaDurMax = 34;
    } else if (c == MAGO) {
        player.vidaMax = 65;
        player.vida = player.vidaMax;
        player.atr.forca = 5;
        player.atr.agilidade = 2;
        player.atr.vitalidade = 1;
        player.atr.armadura = 1;
        player.armaDurMax = 26;
    } else if (c == LADINO) {
        player.vidaMax = 75;
        player.vida = player.vidaMax;
        player.atr.forca = 4;
        player.atr.agilidade = 4;
        player.atr.vitalidade = 2;
        player.atr.armadura = 1;
        player.imuneArmadilha = true;
        player.armaDurMax = 30;
    }
    player.armaDur = player.armaDurMax;
}

void GerarMapa(int nivel) {
    inimigos.clear();
    itens.clear();
    bossDerrotado = false;
    chaveLiberada = false;

    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            visto[y][x] = false;
            if (x == 0 || y == 0 || x == MAP_W-1 || y == MAP_H-1) mapa[y][x] = PAREDE;
            else mapa[y][x] = CHAO;
        }
    }

    // Paredes internas simples com corredores.
    for (int y = 2; y < MAP_H-2; y += 4) {
        for (int x = 3; x < MAP_W-3; x++) {
            if (x % 7 != 0) mapa[y][x] = PAREDE;
        }
    }
    for (int x = 5; x < MAP_W-4; x += 8) {
        for (int y = 4; y < MAP_H-3; y++) {
            if (y % 5 != 0) mapa[y][x] = PAREDE;
        }
    }

    // Obstaculos e armadilhas.
    for (int i = 0; i < 12 + nivel * 2; i++) {
        int x = 2 + rand() % (MAP_W-4);
        int y = 2 + rand() % (MAP_H-4);
        if (mapa[y][x] == CHAO && !(x < 4 && y < 4)) mapa[y][x] = ARMADILHA;
    }

    mapa[1][1] = CHAO;
    mapa[1][2] = CHAO;
    mapa[2][1] = CHAO;
    mapa[2][2] = CHAO;

    mapa[MAP_H-2][MAP_W-2] = SAIDA;
    mapa[MAP_H-2][MAP_W-3] = PORTA;

    npc.pos = CentroTile(3, MAP_H-3);
    mapa[MAP_H-3][3] = NPC_TILE;
    npc.falou = false;
    npc.abriuPassagem = false;

    // Itens.
    for (int i = 0; i < 7 + nivel; i++) {
        int x, y;
        do {
            x = 2 + rand() % (MAP_W-4);
            y = 2 + rand() % (MAP_H-4);
        } while (mapa[y][x] != CHAO);
        Item it;
        it.pos = CentroTile(x, y);
        int tiposSemChave[3] = {0, 2, 3};
        it.tipo = tiposSemChave[i % 3];
        itens.push_back(it);
    }

    // Inimigos normais.
    for (int i = 0; i < 3 + nivel; i++){
        int x, y;
        do {
            x = 3 + rand() % (MAP_W-6);
            y = 3 + rand() % (MAP_H-6);
        } while (mapa[y][x] != CHAO || Dist(CentroTile(x,y), player.pos) < 160);

        Inimigo e;
        e.pos = CentroTile(x, y);
        e.vidaMax = 40 + nivel * 9;
        e.vida = e.vidaMax;
        e.dano = 7 + nivel * 2;
        e.vel = 50 + nivel * 4;
        e.persegue = (i % 2 == 0);
        e.boss = false;
        e.dir = {1, 0};
        inimigos.push_back(e);
    }

    // Boss a cada 3 fases.
    if (FaseComBoss(nivel)) {
        Inimigo b;
        b.pos = CentroTile(MAP_W-5, MAP_H-5);
        b.vidaMax = 120 + nivel * 40;
        b.vida = b.vidaMax;
        b.dano = 12 + nivel * 3;
        b.vel = 60 + nivel * 3;
        b.persegue = true;
        b.boss = true;
        b.dir = {-1, 0};
        inimigos.push_back(b);
    }

    RevelarMapa();
}

void NovoJogo(ClasseJogador classeEscolhida) {
    player = Jogador();
    AplicarClasse(classeEscolhida);
    player.pos = CentroTile(1, 1);
    nivelAtual = 1;
    GerarMapa(nivelAtual);
    tela = JOGO;
    SetMensagem("Classe escolhida: " + player.nomeClasse + ". Entre na masmorra e sobreviva.");
}

void TentarMover(Vector2 delta, float dt) {
    if (delta.x == 0 && delta.y == 0) return;

    float len = sqrtf(delta.x*delta.x + delta.y*delta.y);
    delta.x /= len;
    delta.y /= len;

    // Normalizacao diagonal: W+D nao fica mais rapido.
    float velocidade = 120.0f + player.atr.agilidade * 8.0f;
    Vector2 novaX = {player.pos.x + delta.x * velocidade * dt, player.pos.y};
    Vector2 novaY = {player.pos.x, player.pos.y + delta.y * velocidade * dt};

    if (!PosicaoBloqueada(novaX, player.raio)) player.pos.x = novaX.x;
    if (!PosicaoBloqueada(novaY, player.raio)) player.pos.y = novaY.y;

    player.movimentos++;
    player.score = max(0, player.score - 1);
}

void ColetarItens() {
    for (Item &it : itens) {
        if (it.ativo && Dist(player.pos, it.pos) < 24) {
            it.ativo = false;
            player.score += 60;
            if (it.tipo == 0) {
                player.pocao++;
                SetMensagem("Pocao coletada! Use na batalha para gastar uma acao e recuperar vida.");
            } else if (it.tipo == 1) {
                player.chaves++;
                SetMensagem("Chave coletada! Portas podem ser abertas.");
            } else if (it.tipo == 2) {
                player.escudo = true;
                SetMensagem("Escudo equipado! O proximo dano sera reduzido.");
            } else {
                player.xp += 20;
                SetMensagem("Cristal de experiencia coletado!");
            }
        }
    }
}

void AbrirPortas() {
    int px = (int)(player.pos.x / TILE);
    int py = (int)(player.pos.y / TILE);
    for (int y = py-1; y <= py+1; y++) {
        for (int x = px-1; x <= px+1; x++) {
            if (DentroMapa(x,y) && mapa[y][x] == PORTA && player.chaves > 0) {
                mapa[y][x] = CHAO;
                player.score += 120;
                SetMensagem("Porta aberta. A chave sera usada na saida.");
            }
        }
    }
}

void InteragirNpc() {
    if (tela == SAFE_ROOM && Dist(player.pos, npc.pos) < 55) {
        npc.falou = true;
        tela = LOJA;
        SetMensagem("NPC vendedor: escolha o que deseja comprar.");
    } else if (tela == SAFE_ROOM) {
        SetMensagem("Chegue mais perto do NPC vendedor para interagir.");
    }
}

void ComprarLoja(int op) {
    if (op == 1) {
        if (player.dinheiro >= 25) { player.dinheiro -= 25; player.pocao++; SetMensagem("Comprou uma pocao."); }
        else SetMensagem("Dinheiro insuficiente para pocao.");
    } else if (op == 2) {
        if (player.dinheiro >= 40) { player.dinheiro -= 40; player.vida = player.vidaMax; SetMensagem("Vida totalmente curada."); }
        else SetMensagem("Dinheiro insuficiente para curar.");
    } else if (op == 3) {
        if (player.dinheiro >= 35) { player.dinheiro -= 35; player.escudo = true; SetMensagem("Escudo restaurado."); }
        else SetMensagem("Dinheiro insuficiente para escudo.");
    } else if (op == 4) {
        if (player.dinheiro >= 30) { player.dinheiro -= 30; player.armaDur = player.armaDurMax; SetMensagem("Arma restaurada."); }
        else SetMensagem("Dinheiro insuficiente para restaurar arma.");
    } else if (op == 5) {
        if (player.dinheiro >= 60) { player.dinheiro -= 60; player.atr.forca++; player.armaDurMax += 4; player.armaDur = player.armaDurMax; SetMensagem("Melhoria comprada: mais forca e durabilidade."); }
        else SetMensagem("Dinheiro insuficiente para melhoria.");
    }
}

void SairLojaEAvancar() {
    if (nivelAtual < MAX_FASES) {
        nivelAtual++;
        player.pos = CentroTile(1, 1);
        GerarMapa(nivelAtual);
        tela = JOGO;
        SetMensagem("Voce saiu da SAFE ROOM. Fase avancada!");
    } else {
        tela = VITORIA;
    }
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
            bossDerrotado = true;
            AplicarBonusBoss();
            SetMensagem(TextFormat("Boss derrotado! +%d moedas. Bonus aplicado e SAFE ROOM liberada.", recompensa));
        } else {
            SetMensagem(TextFormat("Inimigo derrotado! +%d moedas.", recompensa));
        }
    }
    inimigoBatalha = -1;
    textoBatalha = "";
    if (player.vida <= 0) tela = DERROTA;
    else if (tela != LEVELUP) tela = JOGO;
}

void IniciarBatalha(int idx) {
    if (idx < 0 || idx >= (int)inimigos.size()) return;
    if (inimigos[idx].vida <= 0) return;
    inimigoBatalha = idx;
    tela = BATALHA;
    textoBatalha = inimigos[idx].boss ? "O chefe apareceu! Escolha sua acao." : "Um inimigo apareceu! Escolha sua acao.";
}

void Atacar() {
    // Agora o ataque fora da batalha serve apenas para iniciar o combate com inimigo proximo.
    for (int i = 0; i < (int)inimigos.size(); i++) {
        if (inimigos[i].vida > 0 && Dist(player.pos, inimigos[i].pos) < 58) {
            IniciarBatalha(i);
            return;
        }
    }
    SetMensagem("Nenhum inimigo perto para batalhar.");
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
    textoBatalha += TextFormat(" Inimigo causou %d de dano.", dano);

    if (player.vida <= 0) tela = DERROTA;
}

void AcaoBatalha(int opcao) {
    if (inimigoBatalha < 0 || inimigoBatalha >= (int)inimigos.size()) {
        tela = JOGO;
        return;
    }

    Inimigo &e = inimigos[inimigoBatalha];

    if (opcao == 1) {
        int danoBase = 0;

        if (player.classe == GUERREIRO)
            danoBase = 9;
        else if (player.classe == MAGO)
            danoBase = 13;
        else if (player.classe == LADINO)
            danoBase = 10;
        else
            danoBase = 8;

        int dano = danoBase + (player.atr.forca * 3) / 2 + player.nivel;

        if (player.armaDur <= player.armaDurMax / 3)
            dano = dano * 70 / 100;

        bool criticoArcano = false;
        if (player.classe == MAGO && (rand() % 100) < 20) {
            dano = dano * 3 / 2;
            criticoArcano = true;
        }

        int acerto = 70 + player.atr.agilidade * 4;

        if (player.armaDur > 0)
            player.armaDur--;

        if ((rand() % 100) < acerto) {
            e.vida -= dano;
            player.score += 25;
            if (criticoArcano)
                textoBatalha = TextFormat("Passiva do Mago ativou! Dano arcano critico: %d. Durabilidade: %d/%d", dano, player.armaDur, player.armaDurMax);
            else
                textoBatalha = TextFormat("Voce atacou e causou %d de dano! Durabilidade: %d/%d", dano, player.armaDur, player.armaDurMax);
            if (e.vida <= 0) {
                EncerrarBatalha(true);
                return;
            }
        } else {
            textoBatalha = "Voce atacou, mas errou!";
        }
        TurnoInimigo();
    }

    if (opcao == 2) {
        if (player.pocao > 0) {
            player.pocao--;
            int cura = 45 + player.atr.vitalidade * 5;
            player.vida = min(player.vidaMax, player.vida + cura);
            textoBatalha = TextFormat("Voce usou uma pocao e recuperou vida.");
            TurnoInimigo();
        } else if (!player.escudo) {
            player.escudo = true;
            textoBatalha = "Sem pocoes. Voce preparou um escudo defensivo.";
            TurnoInimigo();
        } else {
            textoBatalha = "Voce nao tem itens de cura disponiveis.";
        }
    }

    if (opcao == 3) {
        if (e.boss) {
            textoBatalha = "Nao da para fugir do chefe final!";
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

void AtualizarInimigos(float dt) {
    for (Inimigo &e : inimigos) {
        if (e.vida <= 0) continue;

        Vector2 dir = {0,0};
        float d = Dist(e.pos, player.pos);

        if (e.persegue && d < 260) {
            dir = {player.pos.x - e.pos.x, player.pos.y - e.pos.y};
        } else {
            e.trocaDir -= dt;
            if (e.trocaDir <= 0) {
                int r = rand() % 8;
                float dx[8] = {1,-1,0,0,1,1,-1,-1};
                float dy[8] = {0,0,1,-1,1,-1,1,-1};
                e.dir = {dx[r], dy[r]};
                e.trocaDir = 1.0f + (rand()%100)/60.0f;
            }
            dir = e.dir;
        }

        float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
        if (len > 0) {
            dir.x /= len;
            dir.y /= len;
        }

        Vector2 nova = {e.pos.x + dir.x * e.vel * dt, e.pos.y + dir.y * e.vel * dt};
        if (!PosicaoBloqueada(nova, e.boss ? 16 : 12)) {
            e.pos = nova;
        } else {
            e.trocaDir = 0;
        }

        if (d < player.raio + (e.boss ? 22 : 14)) {
            int idx = (int)(&e - &inimigos[0]);
            IniciarBatalha(idx);
            return;
        }
    }
}
bool TodosInimigosMortos() {
    for (const Inimigo &e : inimigos) {
        if (e.vida > 0) {
            return false;
        }
    }
    return true;
}

void SpawnarChave() {
    int x, y;

    do {
        x = 2 + rand() % (MAP_W - 4);
        y = 2 + rand() % (MAP_H - 4);
    } while (mapa[y][x] != CHAO);

    Item chave;
    chave.pos = CentroTile(x, y);
    chave.tipo = 1;
    chave.ativo = true;
    itens.push_back(chave);
}
void AtualizarJogo(float dt) {
    if (player.ataqueCooldown > 0) player.ataqueCooldown -= dt;
    if (msgTimer > 0) msgTimer -= dt;

    Vector2 mov = {0,0};
    if (IsKeyDown(KEY_W)) mov.y -= 1;
    if (IsKeyDown(KEY_S)) mov.y += 1;
    if (IsKeyDown(KEY_A)) mov.x -= 1;
    if (IsKeyDown(KEY_D)) mov.x += 1;
    TentarMover(mov, dt);

    if (IsKeyPressed(KEY_SPACE)) Atacar();
    if (IsKeyPressed(KEY_E)) {
        AbrirPortas();
        InteragirNpc();
    }

    ColetarItens();
    AtualizarInimigos(dt);
    RevelarMapa();

    if(!chaveLiberada && TodosInimigosMortos()) {
        SpawnarChave();
        chaveLiberada = true;
        SetMensagem("Todos os inimigos foram derrotados! Uma chave apareceu no mapa.");
    }

    int tx = (int)(player.pos.x / TILE);
    int ty = (int)(player.pos.y / TILE);

    if (DentroMapa(tx, ty) && mapa[ty][tx] == ARMADILHA) {
        if (player.imuneArmadilha) {
            mapa[ty][tx] = CHAO;
            SetMensagem("Ladino desarmou a armadilha sem sofrer dano.");
        } else {
            int dano = max(1, 12 + nivelAtual - player.atr.armadura);
            player.vida -= dano;
            mapa[ty][tx] = CHAO;
            player.score = max(0, player.score - 50);
            SetMensagem("Armadilha ativada! O mapa foi alterado.");
        }
    }


    if (player.vida <= 0) tela = DERROTA;

   if (DentroMapa(tx, ty) && mapa[ty][tx] == SAIDA) {
    if (player.chaves <= 0) {
        SetMensagem("A saida esta trancada. Derrote todos os inimigos e pegue a chave.");
    } else if (FaseComBoss(nivelAtual) && !bossDerrotado) {
        SetMensagem("Derrote o boss desta fase antes de sair!");
    } else if (FaseComBoss(nivelAtual) && bossDerrotado) {
        player.chaves--;
        GerarSafeRoom();
        tela = SAFE_ROOM;
    } else if (nivelAtual < MAX_FASES) {
        player.chaves--;
        nivelAtual++;
        player.pos = CentroTile(1, 1);
        GerarMapa(nivelAtual);
        SetMensagem("Fase avancada! Inimigos ficaram mais fortes.");
    } else {
        player.chaves--;
        tela = VITORIA;
    }
}

}


void AtualizarSafeRoom(float dt) {
    if (msgTimer > 0) msgTimer -= dt;

    Vector2 mov = {0,0};
    if (IsKeyDown(KEY_W)) mov.y -= 1;
    if (IsKeyDown(KEY_S)) mov.y += 1;
    if (IsKeyDown(KEY_A)) mov.x -= 1;
    if (IsKeyDown(KEY_D)) mov.x += 1;

    TentarMover(mov, dt);
    RevelarMapa();

    if (IsKeyPressed(KEY_E)) {
        InteragirNpc();
    }
}

void DesenharTextoCentral(const string& texto, int y, int tamanho, Color cor) {
    int w = MeasureText(texto.c_str(), tamanho);
    DrawText(texto.c_str(), SCREEN_W/2 - w/2, y, tamanho, cor);
}

void DesenharMapa() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            Rectangle r = {(float)x*TILE, (float)y*TILE, (float)TILE, (float)TILE};

            if (!visto[y][x]) {
                DrawRectangleRec(r, BLACK);
                continue;
            }

            Color c = {35,35,42,255};
            if (mapa[y][x] == PAREDE) c = {90,85,100,255};
            if (mapa[y][x] == PORTA) c = BROWN;
            if (mapa[y][x] == ARMADILHA) c = DARKPURPLE;
            if (mapa[y][x] == SAIDA) c = DARKGREEN;
            if (mapa[y][x] == NPC_TILE) c = SKYBLUE;

            DrawRectangleRec(r, c);
            DrawRectangleLines(x*TILE, y*TILE, TILE, TILE, {20,20,25,255});
        }
    }
}

void DesenharJogo() {
    DesenharMapa();

    for (const Item &it : itens) {
        if (!it.ativo) continue;
        int x = (int)(it.pos.x / TILE);
        int y = (int)(it.pos.y / TILE);
        if (!visto[y][x]) continue;

        Color c = RED;
        const char* s = "P";
        if (it.tipo == 1) { c = GOLD; s = "K"; }
        if (it.tipo == 2) { c = BLUE; s = "S"; }
        if (it.tipo == 3) { c = VIOLET; s = "XP"; }
        DrawCircleV(it.pos, 10, c);
        DrawText(s, (int)it.pos.x-7, (int)it.pos.y-7, 14, WHITE);
    }

    for (const Inimigo &e : inimigos) {
        if (e.vida <= 0) continue;
        int x = (int)(e.pos.x / TILE);
        int y = (int)(e.pos.y / TILE);
        if (!DentroMapa(x,y) || !visto[y][x]) continue;

        DrawCircleV(e.pos, e.boss ? 22 : 13, e.boss ? MAROON : ORANGE);
        DrawRectangle((int)e.pos.x-18, (int)e.pos.y-25, 36, 5, DARKGRAY);
        DrawRectangle((int)e.pos.x-18, (int)e.pos.y-25, (int)(36.0f*e.vida/e.vidaMax), 5, RED);
    }

    DrawCircleV(player.pos, player.raio, player.cor);
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, (int)player.raio, player.escudo ? BLUE : DARKGRAY);

    if (player.ataqueCooldown > 0.18f) {
        DrawCircleLines((int)player.pos.x, (int)player.pos.y, 52, YELLOW);
    }

    DrawRectangle(0, MAP_H*TILE, SCREEN_W, 90, {15,15,20,255});
    DrawText(TextFormat("Fase: %d/%d | Classe: %s | HP: %d/%d | $: %d | Arma: %d/%d | Pocoes: %d",
             nivelAtual, MAX_FASES, player.nomeClasse.c_str(), player.vida, player.vidaMax, player.dinheiro, player.armaDur, player.armaDurMax, player.pocao),
             10, MAP_H*TILE + 10, 20, RAYWHITE);
    DrawText(TextFormat("FOR %d  AGI %d  VIT %d  ARM %d | Score: %d | Movimentos: %d",
             player.atr.forca, player.atr.agilidade, player.atr.vitalidade, player.atr.armadura, player.score, player.movimentos),
             10, MAP_H*TILE + 38, 20, RAYWHITE);
    DrawText("WASD: mover  |  SPACE: batalha perto do inimigo  |  E: interagir/abrir porta  |  ESC: menu",
             10, MAP_H*TILE + 64, 18, GRAY);

    if (msgTimer > 0) {
        DrawRectangle(190, 12, SCREEN_W-380, 34, {0,0,0,180});
        DesenharTextoCentral(mensagem, 18, 18, YELLOW);
    }
}


void DesenharSafeRoom() {
    DesenharMapa();

    // NPC vendedor
    DrawCircleV(npc.pos, 15, SKYBLUE);
    DrawText("$", (int)npc.pos.x - 5, (int)npc.pos.y - 10, 22, BLACK);

    DrawCircleV(player.pos, player.raio, player.cor);
    DrawCircleLines((int)player.pos.x, (int)player.pos.y, (int)player.raio, player.escudo ? BLUE : DARKGRAY);

    DrawRectangle(0, MAP_H*TILE, SCREEN_W, 90, {12,18,24,255});
    DrawText(TextFormat("SAFE ROOM | Fase concluida: %d | Classe: %s | HP: %d/%d | $: %d | Arma: %d/%d",
             nivelAtual, player.nomeClasse.c_str(), player.vida, player.vidaMax, player.dinheiro, player.armaDur, player.armaDurMax),
             10, MAP_H*TILE + 10, 20, RAYWHITE);
    DrawText("WASD: mover  |  E perto do NPC: abrir loja  |  A sala segura nao possui inimigos",
             10, MAP_H*TILE + 42, 20, GRAY);

    if (msgTimer > 0) {
        DrawRectangle(170, 12, SCREEN_W-340, 34, {0,0,0,180});
        DesenharTextoCentral(mensagem, 18, 18, YELLOW);
    }
}

void DesenharBatalha() {
    ClearBackground({12,12,18,255});

    if (inimigoBatalha < 0 || inimigoBatalha >= (int)inimigos.size()) {
        DesenharTextoCentral("BATALHA", 80, 34, RAYWHITE);
        DesenharTextoCentral("Nenhum inimigo encontrado.", 150, 22, GRAY);
        return;
    }

    Inimigo &e = inimigos[inimigoBatalha];

    DesenharTextoCentral(e.boss ? "BATALHA FINAL" : "BATALHA", 50, 34, e.boss ? RED : RAYWHITE);

    DrawRectangle(80, 120, SCREEN_W - 160, 120, {30,30,42,255});
    DrawRectangleLines(80, 120, SCREEN_W - 160, 120, GRAY);
    DrawText(e.boss ? "CHEFE DA CRIPTA" : "INIMIGO DA MASMORRA", 110, 145, 24, e.boss ? RED : ORANGE);
    DrawText(TextFormat("Vida do inimigo: %d/%d", max(0, e.vida), e.vidaMax), 110, 185, 22, RAYWHITE);

    DrawRectangle(80, 270, SCREEN_W - 160, 120, {25,35,30,255});
    DrawRectangleLines(80, 270, SCREEN_W - 160, 120, GRAY);
    DrawText("JOGADOR", 110, 295, 24, GREEN);
    DrawText(TextFormat("HP: %d/%d | Pocoes: %d | Escudo: %s | Arma: %d/%d | $: %d", max(0, player.vida), player.vidaMax, player.pocao, player.escudo ? "SIM" : "NAO", player.armaDur, player.armaDurMax, player.dinheiro), 110, 335, 22, RAYWHITE);

    DrawRectangle(80, 430, SCREEN_W - 160, 150, {20,20,28,255});
    DrawRectangleLines(80, 430, SCREEN_W - 160, 150, GRAY);
    DrawText("Escolha uma acao:", 110, 455, 24, RAYWHITE);
    DrawText("[1] Atacar", 130, 495, 22, YELLOW);
    DrawText("[2] Curar / Usar item", 330, 495, 22, SKYBLUE);
    DrawText(e.boss ? "[3] Sair/Fugir bloqueado no chefe" : "[3] Sair / Fugir", 620, 495, 22, ORANGE);

    if (!textoBatalha.empty()) {
        DrawText(textoBatalha.c_str(), 110, 610, 20, GRAY);
    }
}


void DesenharSeletorClasses() {
    ClearBackground({12,12,18,255});
    DesenharTextoCentral("ESCOLHA SUA CLASSE", 60, 34, GOLD);
    DesenharTextoCentral("Cada classe muda vida, dano, defesa e cor da bolinha no mapa.", 105, 19, GRAY);

    DrawRectangle(80, 160, 250, 300, {25,30,45,255});
    DrawRectangleLines(80, 160, 250, 300, BLUE);
    DrawCircle(205, 205, 18, BLUE);
    DrawText("[1] GUERREIRO", 110, 245, 24, RAYWHITE);
    DrawText("Vida alta", 110, 285, 20, GRAY);
    DrawText("Dano medio", 110, 315, 20, GRAY);
    DrawText("Comeca com escudo", 110, 345, 20, GRAY);

    DrawRectangle(355, 160, 250, 300, {35,25,45,255});
    DrawRectangleLines(355, 160, 250, 300, VIOLET);
    DrawCircle(480, 205, 18, VIOLET);
    DrawText("[2] MAGO", 405, 245, 24, RAYWHITE);
    DrawText("Vida baixa", 405, 285, 20, GRAY);
    DrawText("Dano alto", 405, 315, 20, GRAY);
    DrawText("Passiva: critico arcano", 405, 345, 20, GRAY);

    DrawRectangle(630, 160, 250, 300, {25,45,30,255});
    DrawRectangleLines(630, 160, 250, 300, LIME);
    DrawCircle(755, 205, 18, LIME);
    DrawText("[3] LADINO", 680, 245, 24, RAYWHITE);
    DrawText("Vida baixa", 680, 285, 20, GRAY);
    DrawText("Bom dano/agilidade", 680, 315, 20, GRAY);
    DrawText("Imune a armadilhas", 680, 345, 20, GRAY);

    DesenharTextoCentral("ESC volta ao menu", 540, 20, GRAY);
}

void DesenharLoja() {
    ClearBackground({10,16,22,255});
    DesenharTextoCentral("SALA SEGURA - NPC VENDEDOR", 55, 32, SKYBLUE);
    DesenharTextoCentral(TextFormat("Moedas: %d | HP: %d/%d | Arma: %d/%d | Pocoes: %d", player.dinheiro, player.vida, player.vidaMax, player.armaDur, player.armaDurMax, player.pocao), 105, 21, GOLD);
    DrawRectangle(150, 160, SCREEN_W - 300, 330, {25,25,34,255});
    DrawRectangleLines(150, 160, SCREEN_W - 300, 330, GRAY);
    DrawText("[1] Comprar pocao - 25 moedas", 190, 205, 24, RAYWHITE);
    DrawText("[2] Curar toda a vida - 40 moedas", 190, 250, 24, RAYWHITE);
    DrawText("[3] Comprar/restaurar escudo - 35 moedas", 190, 295, 24, RAYWHITE);
    DrawText("[4] Restaurar durabilidade da arma - 30 moedas", 190, 340, 24, RAYWHITE);
    DrawText("[5] Melhorar arma e forca - 60 moedas", 190, 385, 24, RAYWHITE);
    DrawText("[ENTER] Sair da SAFE ROOM e ir para a proxima fase", 190, 440, 24, GREEN);
    if (msgTimer > 0) DesenharTextoCentral(mensagem, 535, 20, YELLOW);
}

void DesenharMenu() {
    ClearBackground({12,12,18,255});
    DesenharTextoCentral("ROGUE DA CRIPTA PERDIDA", 95, 34, RAYWHITE);
    DesenharTextoCentral("Uma masmorra viva, cheia de chaves, armadilhas, NPCs e um chefe final.", 145, 18, GRAY);
    DesenharTextoCentral("[ENTER] Escolher classe", 230, 24, GREEN);
    DesenharTextoCentral("[1] Como funciona", 270, 22, RAYWHITE);
    DesenharTextoCentral("[2] Itens", 305, 22, RAYWHITE);
    DesenharTextoCentral("[3] Pontuacao", 340, 22, RAYWHITE);
    DesenharTextoCentral("[ESC] Sair", 390, 20, GRAY);
}

void DesenharTelaTexto(const string& titulo, const vector<string>& linhas) {
    ClearBackground({12,12,18,255});
    DesenharTextoCentral(titulo, 55, 30, RAYWHITE);
    int y = 120;
    for (const string &l : linhas) {
        DrawText(l.c_str(), 70, y, 20, GRAY);
        y += 34;
    }
    DesenharTextoCentral("[ESC] Voltar", SCREEN_H-60, 20, GREEN);
}

void DistribuirAtributo(int op) {
    if (player.pontosAtributo <= 0) return;
    if (op == 1) player.atr.forca++;
    if (op == 2) {
        player.atr.agilidade++;
    }
    if (op == 3) {
        player.atr.vitalidade++;
        player.vidaMax += 15;
        player.vida += 15;
    }
    if (op == 4) player.atr.armadura++;
    player.pontosAtributo--;
    if (player.pontosAtributo <= 0) tela = JOGO;
}

int main() {
    srand((unsigned)time(nullptr));

    InitWindow(SCREEN_W, SCREEN_H, "Roguelike C++ - Cripta Perdida");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (tela == MENU) {
            if (IsKeyPressed(KEY_ENTER)) tela = CLASSES;
            if (IsKeyPressed(KEY_ONE)) tela = AJUDA;
            if (IsKeyPressed(KEY_TWO)) tela = ITENS;
            if (IsKeyPressed(KEY_THREE)) tela = PONTUACAO;
        } else if (tela == CLASSES) {
            if (IsKeyPressed(KEY_ESCAPE)) tela = MENU;
            if (IsKeyPressed(KEY_ONE)) NovoJogo(GUERREIRO);
            if (IsKeyPressed(KEY_TWO)) NovoJogo(MAGO);
            if (IsKeyPressed(KEY_THREE)) NovoJogo(LADINO);
        } else if (tela == JOGO) {
            if (IsKeyPressed(KEY_ESCAPE)) tela = MENU;
            else AtualizarJogo(dt);
        } else if (tela == SAFE_ROOM) {
            if (IsKeyPressed(KEY_ESCAPE)) tela = MENU;
            else AtualizarSafeRoom(dt);
        } else if (tela == LEVELUP) {
            if (IsKeyPressed(KEY_ONE)) DistribuirAtributo(1);
            if (IsKeyPressed(KEY_TWO)) DistribuirAtributo(2);
            if (IsKeyPressed(KEY_THREE)) DistribuirAtributo(3);
            if (IsKeyPressed(KEY_FOUR)) DistribuirAtributo(4);
        } else if (tela == BATALHA) {
            if (IsKeyPressed(KEY_ONE)) AcaoBatalha(1);
            if (IsKeyPressed(KEY_TWO)) AcaoBatalha(2);
            if (IsKeyPressed(KEY_THREE)) AcaoBatalha(3);
        } else if (tela == LOJA) {
            if (msgTimer > 0) msgTimer -= dt;
            if (IsKeyPressed(KEY_ONE)) ComprarLoja(1);
            if (IsKeyPressed(KEY_TWO)) ComprarLoja(2);
            if (IsKeyPressed(KEY_THREE)) ComprarLoja(3);
            if (IsKeyPressed(KEY_FOUR)) ComprarLoja(4);
            if (IsKeyPressed(KEY_FIVE)) ComprarLoja(5);
            if (IsKeyPressed(KEY_ENTER)) SairLojaEAvancar();
        } else {
            if (IsKeyPressed(KEY_ESCAPE)) tela = MENU;
            if ((tela == VITORIA || tela == DERROTA) && IsKeyPressed(KEY_ENTER)) tela = CLASSES;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (tela == MENU) DesenharMenu();
        if (tela == CLASSES) DesenharSeletorClasses();
        if (tela == JOGO) DesenharJogo();
        if (tela == SAFE_ROOM) DesenharSafeRoom();
        if (tela == BATALHA) DesenharBatalha();
        if (tela == LOJA) DesenharLoja();

        if (tela == AJUDA) {
            DesenharTelaTexto("COMO FUNCIONA", {
                "Explore 9 fases, revele o mapa escuro e sobreviva ate o boss final.",
                "O jogador se move em 8 direcoes com WASD, incluindo diagonais fluidas.",
                "Use SPACE perto de inimigos para entrar em batalha por turno.",
                "Inimigos respeitam colisao com paredes; alguns perseguem o jogador.",
                "A cada 3 fases existe um boss; depois dele abre uma SAFE ROOM com vendedor."
            });
        }

        if (tela == ITENS) {
            DesenharTelaTexto("ITENS", {
                "P vermelho: pocao de vida, usada em batalha e consome a rodada.",
                "K dourado: chave liberada apos derrotar todos os inimigos da fase.",
                "S azul: escudo, reduz o proximo dano recebido.",
                "XP roxo: cristal que concede experiencia.",
                "Armadilhas roxas causam dano, exceto para o Ladino."
            });
        }

        if (tela == PONTUACAO) {
            DesenharTelaTexto("SISTEMA DE PONTUACAO", {
                "+ pontos por itens, portas, inimigos, bosses e progresso de fase.",
                "+ dinheiro ao derrotar inimigos; bosses dao recompensas maiores.",
                "- pontos por excesso de movimentos e por ativar armadilhas.",
                "A tela mostra vida, classe, dinheiro, arma, pontos, chaves e progresso."
            });
        }

        if (tela == LEVELUP) {
            ClearBackground({12,12,18,255});
            DesenharTextoCentral("SUBIU DE NIVEL!", 90, 34, GOLD);
            DesenharTextoCentral(TextFormat("Pontos disponiveis: %d", player.pontosAtributo), 145, 24, RAYWHITE);
            DesenharTextoCentral("[1] Forca: aumenta dano", 220, 22, RAYWHITE);
            DesenharTextoCentral("[2] Agilidade: aumenta velocidade, acerto e esquiva", 260, 22, RAYWHITE);
            DesenharTextoCentral("[3] Vitalidade: aumenta vida maxima", 300, 22, RAYWHITE);
            DesenharTextoCentral("[4] Armadura: reduz dano recebido", 340, 22, RAYWHITE);
        }

        if (tela == VITORIA) {
            ClearBackground({10,28,16,255});
            DesenharTextoCentral("VITORIA!", 120, 42, GREEN);
            DesenharTextoCentral("Voce superou as 9 fases, venceu os bosses e libertou a Cripta Perdida.", 180, 22, RAYWHITE);
            DesenharTextoCentral(TextFormat("Pontuacao final: %d", player.score), 230, 26, GOLD);
            DesenharTextoCentral("[ENTER] Jogar novamente  |  [ESC] Menu", 300, 22, GRAY);
        }

        if (tela == DERROTA) {
            ClearBackground({30,8,8,255});
            DesenharTextoCentral("DERROTA", 120, 42, RED);
            DesenharTextoCentral("Sua vida chegou a zero. A morte permanente venceu desta vez.", 180, 22, RAYWHITE);
            DesenharTextoCentral(TextFormat("Pontuacao final: %d", player.score), 230, 26, GOLD);
            DesenharTextoCentral("[ENTER] Tentar novamente  |  [ESC] Menu", 300, 22, GRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
