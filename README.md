# Projeto Integrador 3
<img src="https://user-images.githubusercontent.com/17687969/207394502-023f39c8-d682-4d9e-b9fc-01e98b6d5a4a.jpg" alt="drawing" width="50%"/>

## Concepção:

O projeto consiste em implementar um produto já existente, porém de custo proibitivo:

<img src="https://user-images.githubusercontent.com/17687969/207396438-5dea248a-152a-49d1-897b-841934cdbb2c.png" alt="drawing" width="50%"/>

Este é uma placar de Esgrima, para a modalidade Épée, sua função é avaliar se o ataque da espada dos jogadores foi válido.

O objvetivo é implemantar esta funcionalidade com o menor curto possível.

## Desenvolvimento:

Construção padrão da espada:

![image](https://user-images.githubusercontent.com/17687969/207399514-a4f31b9b-77c6-456a-bf90-6ee5c1d06391.png)

A espada elétrica possui 3 contatos:
* Guard: É conectado ao terra, também está presente no cabo da espada, fazendo contado direto com o corpo do jogador
* Tip e Return Tip: Eles conectam um push button que está na ponta da espada leitura, enviando e recebando um sinal

### Regras de funcionamento do equipamento oficial:
O sistema oficial, que é ligado através de cabos conectados nas costas do jogadores e mantido tensionado por um sistema de polias, é monitorado á uma velocidade de 500ms, ou seja, o tempo máximo de detecção é de 2ms, o sistema também indetifica empates, quando os dois Jogadores obtém um toque válido em um invertavo inferior á 40ms, acendendo os dois lados do placar


