# Projeto Integrador 3
<img src="https://user-images.githubusercontent.com/17687969/207394502-023f39c8-d682-4d9e-b9fc-01e98b6d5a4a.jpg" alt="drawing" width="50%"/>

## Concepção:

O projeto consiste em implementar um produto já existente, porém de custo proibitivo:

<img src="https://user-images.githubusercontent.com/17687969/207396438-5dea248a-152a-49d1-897b-841934cdbb2c.png" alt="drawing" width="50%"/>

Este é uma placar de Esgrima, para a modalidade Épée, sua função é avaliar se o ataque da espada dos jogadores foi válido.

O objvetivo é implemantar esta funcionalidade com o menor curto possível.

### Regras de funcionamento do equipamento oficial:

  O sistema oficial, utilizado nas olimpíadas por exemplo, é ligado através de cabos conectados nas costas do jogadores e mantido tensionado por um sistema de polias, este é monitorado á uma velocidade de 500Hz, ou seja, o tempo máximo de detecção é de 2ms, o sistema também indetifica empates, quando os dois jogadores obtém um toque válido em um invertavo inferior á 40ms, acendendo os dois lados do placar

## Desenvolvimento:

Construção padrão da espada:

![image](https://user-images.githubusercontent.com/17687969/207399514-a4f31b9b-77c6-456a-bf90-6ee5c1d06391.png)

A espada elétrica possui 3 contatos:
* Guard: É conectado ao terra, também está presente no cabo da espada, fazendo contado direto com o corpo do jogador
* Tip e Return Tip: Eles conectam um push button que está na ponta da espada leitura, enviando e recebando um sinal



### Analisando o Sistema Comercial:

![image](https://user-images.githubusercontent.com/17687969/207403761-3cff4c21-5a13-4ee1-bafd-dd9cb72e99c3.png)

Esta é a parte analógica do sistema, notasse que ele possui um capacitor variável, que provavelmente é utilizado para a regulação de um ressonador, considerando o indutor ao lado na mesma parte do circuito.

![image](https://user-images.githubusercontent.com/17687969/207403809-dec8dfef-bd0c-455a-9687-8d5b82bf30b4.png)

  Sinal emitido do aparelho capturado pelo osciloscópio, sem a espada, aparentemente a oscilação está sub amortecida, o que índica que se espera alguma capacitância da espada, afim de trazer próximo á ressonância.

![image](https://user-images.githubusercontent.com/17687969/207403854-87014f6a-c9e2-45fd-b77c-828c1e6fdf6c.png)

  Em cinza é a forma de onda padrão, com a espada tocando em uma  não condutiva, e em amarelo é o sinal com menor capacitância possível que pode ser lido, no caso foi utilizado uma forma pequena de alumínio, o pico no momento t=0 é quando se obtém pressão suficiente para fechar o contato na ponta da espada, notasse que a tensão AC antes do toque é a mesma até poucos microssegundos antes do contado ser fechado, indicando que a partir daquele ponto a espada já estava em contato com a superfície metálica antes da detecção da chave, após o pico de tensão, a componente DC estabiliza para próximo de 0V, indicando a ponta pressionada, e a componente AC agora estabilizada, está cerca de 100mV menor que a referência em cinza, indicando que esta queda é o métrica de detecção que este sistema utiliza.

### Escolhendo pricípio de funcionamento:

Como nenhuma patente relacionada ao assunto descrimina os valores médios de capacitância, será explorado a seguinte metodologia:

[Circuits and Techniques for Implementing Capacitive Touch Sensing - Technical Articles (allaboutcircuits.com)](https://www.allaboutcircuits.com/technical-articles/circuits-and-techniques-for-implementing-capacitive-touch-sensing/)

Com esta forma espera-se encontrar uma faixa de queda da constante RC, gerada por uma onda quadrada, que esteja em uma faixa de velocidade e tensão legível para o ADC de um microcontrolador.


Testando as leituras com esp32 conectado ao computador:

![image](https://user-images.githubusercontent.com/17687969/207406633-91598bcb-ceae-4028-9618-7d58fddf2514.png)

Em laranja as medidas diretas do adc, e em vermelho a média dos últimos 500 valores, estes valores são referentes apenas à medida da capacitância parasita da protoboard, em paralelo com um resistor de 1MΩ.

Colocando o esp32 em uma bateria e lendo os valores via rede a partir de outro esp32, exceto pela bateria, a metodologia se manteve a mesma:

![image](https://user-images.githubusercontent.com/17687969/207406713-fd8b4566-701f-48f2-8c86-d113d3336099.png)

Medindo um capacitor de 2.2pF na protoboard:

![image](https://user-images.githubusercontent.com/17687969/207406842-03669e8f-c8c9-48b5-a474-4ff941f58206.png)

Provando que a leitura de baixas capacitâncias por este método é possível


### Novas medidas (peak hold):

Estas medidas foram adiquiridas pelo miconcontrolador, já na bateria, com um resistor de descarga de 100KΩ 

![image](https://user-images.githubusercontent.com/17687969/207407322-c9c9fd02-e622-4755-a109-6d8c2b7c402b.png)

Apesar de uma excelente diferença de valores, ela é majoritáriamenta causada pela capacitância do cabo, pois apenas dois cabos estão conectados até o momento que em que pushbutton é apertado, conectando o terceiro fio ao microcontrolador, causando o aumento de capacitância mesmo para um sinal não condutivo, o sinal real é apenas cerca de 30 pontos ( de 4096 ) entre o sinal condutivo e não condutivo.

Baseado nos valores medidos foi levantado o seguinte modelo:

![image](https://user-images.githubusercontent.com/17687969/207423661-31de1535-7c75-4801-ba0e-e7f2a7e4a22e.png)

Sendo C1, 435pF, a capacitância do cabo, e a capacitância da forma de alumínio está estimada em cerca de 20pF em paralelo com C1

### Mudando estratégia: 
Formar um segundo filtro RC para cancelar os efeitos do terceiro fio sendo ligado em paralelo com o segundo fio, somando essas capacitâncias, para tal basta adicionar um segundo resistor que é acoplado quando o botão da ponta é pressionado.

Eureka! Neste ambiente de testes foi possível notar que a geometria do cabo tem um fator crucial nos valores de capacitância dos fios 2 e 3, mais precisamente, que a relação de capacitância entre eles é sempre a mesma, considerando que ambos tem exatamente o memo comprimento, e, a mesma distância do cabo ligado ao terra, com isso é possível avaliar que a capacitância do fio 3 é sempre cerca de 33% do valor do cabo 2, sabendo disso, basta conectar um resistor de valor 3 vezes R1, neste caso, 300KΩ para de certa forma, cancelar os efeitos de capacitância do fio 3 sobre a medição.

Teste feito com 100kΩ no segundo fio, como retorno de sinal do botão e 300kΩ no terceiro fio, utilizado como medida do filtro RC, o momento em que o ruído cessa é onde o botão foi apertado, em uma superfície não condutiva:

![image](https://user-images.githubusercontent.com/17687969/207409785-2a742c3c-02c4-4e33-af1e-c27619fbe15f.png)

Agora em superfície condutiva:

![image](https://user-images.githubusercontent.com/17687969/207409819-9821eae9-1985-470b-a928-d7fdee006ff6.png)

O resultado final foi este modelo:

![image](https://user-images.githubusercontent.com/17687969/207425338-b1c99e02-5410-409e-b896-acf2a04d06f1.png)




## Referências:

https://patents.google.com/patent/US20060100022

https://patents.google.com/patent/US20010023218A1

https://patents.google.com/patent/US3920242A






