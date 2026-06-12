**# Roguelike - Cripta Perdida

## Sobre o Projeto

Cripta Perdida é um roguelike desenvolvido em C++ utilizando a biblioteca Raylib. O jogador deve explorar uma masmorra repleta de inimigos, armadilhas, itens e bosses enquanto evolui seu personagem e tenta sobreviver até a fase final.

O projeto foi desenvolvido com foco em conceitos de programação estudados na disciplina de Programação em C++, incluindo estruturas, vetores, enumerações, funções, organização modular e lógica de jogos.

---

## Funcionalidades

### Sistema de Classes

O jogador escolhe uma classe antes de iniciar a partida:

#### Guerreiro

* Vida elevada
* Boa resistência
* Começa com escudo
* Dano equilibrado

#### Mago

* Menor quantidade de vida
* Alto dano
* Chance de causar dano crítico
* Classe focada em ataque

#### Ladino

* Alta agilidade
* Bom dano
* Imune a armadilhas
* Maior mobilidade

---

## Sistema de Progressão

* Experiência (XP)
* Sistema de níveis
* Distribuição de atributos
* Dinheiro obtido ao derrotar inimigos
* Loja para compra de melhorias
* Progressão crescente de dificuldade

A cada nível o jogador recebe pontos para melhorar:

* Força
* Agilidade
* Vitalidade
* Armadura

---

## Combate por Turnos

Ao encontrar um inimigo, o jogo entra em um sistema de combate por turnos.

Ações disponíveis:

* Atacar
* Utilizar poção
* Defender-se
* Fugir (exceto contra bosses)

Cada ação realizada consome um turno.

---

## Safe Room

A cada 3 fases existe um Boss.

Após derrotar o Boss:

* Uma Safe Room é liberada
* O jogador pode caminhar até ela
* Existe um NPC vendedor
* É possível comprar melhorias

Na loja podem ser adquiridos:

* Poções
* Cura completa
* Escudo
* Reparo da arma
* Melhorias permanentes

---

## Estrutura das Fases

```text
Fase 1
Fase 2
Fase 3 (Boss)
↓
Safe Room

Fase 4
Fase 5
Fase 6 (Boss)
↓
Safe Room

Fase 7
Fase 8
Fase 9 (Boss Final)
↓
Vitória
```

---

## Controles

| Tecla | Função              |
| ----- | ------------------- |
| W     | Mover para cima     |
| A     | Mover para esquerda |
| S     | Mover para baixo    |
| D     | Mover para direita  |
| SPACE | Iniciar combate     |
| E     | Interagir           |
| ENTER | Confirmar           |
| ESC   | Voltar/Menu         |

---

## Como Compilar

Abra o PowerShell na pasta do projeto:

```powershell
cd C:\Users\SEU_USUARIO\Downloads\T2
```

Configure o compilador da Raylib:

```powershell
$env:Path = "C:\raylib\w64devkit\bin;" + $env:Path
```

Compile:

```powershell
g++ main.cpp -o Roguelike.exe -I"C:\raylib\raylib\src" -L"C:\raylib\raylib\src" -lraylib -lopengl32 -lgdi32 -lwinmm
```

Execute:

```powershell
.\Roguelike.exe
```

---

## Tecnologias Utilizadas

* C++
* Raylib
* Programação Orientada a Objetos
* Estruturas de Dados
* Vetores
* Enumerações
* Sistema de Estados
* Colisão e Movimentação em Tempo Real

---

## Autor

Pedro Henrique Rodrigues

Curso de Ciência da Computação

Universidade do Vale do Itajaí (UNIVALI)
**
