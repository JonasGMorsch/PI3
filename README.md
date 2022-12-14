# Projeto Integrador 3
<img src="https://user-images.githubusercontent.com/17687969/207394502-023f39c8-d682-4d9e-b9fc-01e98b6d5a4a.jpg" alt="drawing" width="50%"/>

## Concepção:

O projeto consiste em implementar um produto já existente, porém de custo proibitivo:

<img src="https://user-images.githubusercontent.com/17687969/207396438-5dea248a-152a-49d1-897b-841934cdbb2c.png" alt="drawing" width="50%"/>

Este é uma placar de Esgrima, para a modalidade Épée, sua função é avaliar se o ataque da espada dos jogadores foi válido.

O objvetivo é implemantar esta funcionalidade com o menor custo possível.

## Design:

O Desing externo terá como base o sistema comercial, já que vem sendo utilizado pelos esportistas que estão acostumados com seu método de funcionamento.
Já o desing interno será construído ao redor do microcontrolador ESP32, que possui capacidade de transmissão sem fio à longa distância, ACD interno, e nessa versão LOLIN32 Lite, também possui capacidade de carregar e funcionar através de uma célula de lítio, que será a fonte de energia para os transmissores dos jogadores.

### Regras de funcionamento do equipamento oficial:

  O sistema oficial, utilizado nas olimpíadas por exemplo, é ligado através de cabos conectados nas costas do jogadores e mantido tensionado por um sistema de polias, este é monitorado á uma velocidade de 500Hz, ou seja, o tempo máximo de detecção é de 2ms, o sistema também indetifica empates, quando os dois jogadores obtém um toque válido em um invertavo inferior á 40ms, acendendo os dois lados do placar

Construção padrão da espada:

![image](https://user-images.githubusercontent.com/17687969/207399514-a4f31b9b-77c6-456a-bf90-6ee5c1d06391.png)

A espada elétrica possui 3 contatos:
* Guard: É conectado ao terra, também está presente no cabo da espada, fazendo contado direto com o corpo do jogador
* Tip e Return Tip: Eles conectam um push button que está na ponta da espada, enviando e recebando sinal

### Analisando o Sistema Comercial:

![image](https://user-images.githubusercontent.com/17687969/207403761-3cff4c21-5a13-4ee1-bafd-dd9cb72e99c3.png)

Esta é a parte analógica do sistema, notasse que ele possui um capacitor variável, que provavelmente é utilizado para a regulação de um ressonador, considerando o indutor ao lado na mesma parte do circuito.

![image](https://user-images.githubusercontent.com/17687969/207403809-dec8dfef-bd0c-455a-9687-8d5b82bf30b4.png)

  Sinal emitido do aparelho capturado pelo osciloscópio, sem a espada, aparentemente a oscilação está sub amortecida, o que índica que se espera alguma capacitância da espada, afim de trazer próximo á ressonância.

![image](https://user-images.githubusercontent.com/17687969/207403854-87014f6a-c9e2-45fd-b77c-828c1e6fdf6c.png)

  Em cinza é a forma de onda padrão, com a espada tocando em uma  não condutiva, e em amarelo é o sinal com menor capacitância possível que pode ser lido, no caso foi utilizado uma forma pequena de alumínio, o pico no momento t=0 é quando se obtém pressão suficiente para fechar o contato na ponta da espada, notasse que a tensão AC antes do toque é a mesma até poucos microssegundos antes do contado ser fechado, indicando que a partir daquele ponto a espada já estava em contato com a superfície metálica antes da detecção da chave, após o pico de tensão, a componente DC estabiliza para próximo de 0V, indicando a ponta pressionada, e a componente AC agora estabilizada, está cerca de 100mV menor que a referência em cinza, indicando que esta queda é o métrica de detecção que este sistema utiliza.

## Implementação:

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


### Novas medidas:

Estas medidas foram adiquiridas pelo miconcontrolador, na bateria, com um resistor de descarga de 100KΩ, utilizando a espada, tocando na forma de alumínio:

![image](https://user-images.githubusercontent.com/17687969/207407322-c9c9fd02-e622-4755-a109-6d8c2b7c402b.png)

Apesar de uma excelente diferença de valores, ela é majoritáriamenta causada pela capacitância do cabo, pois apenas dois cabos estão conectados até o momento que em que pushbutton é apertado, conectando o terceiro fio ao microcontrolador, causando o aumento de capacitância mesmo para um sinal não condutivo, o sinal real é apenas cerca de 30 pontos ( de 4096 ) entre o sinal condutivo e não condutivo.

É importante comentar que o método de filtragem do sinal é diferente após o toque da espada, antes do toque, a linha em laranja é a leitura direta do ADC, e a vermelha a média dos útimos 500 valores, após o toque, a linha laranja apenas aceita valores maiores que o atual (hold peak),e a linha vermelha não é mais atualizada e passa a ser um valor constante, afim de não desviar a média durante o toque.

Baseado nos valores medidos foi levantado o seguinte modelo:

![image](https://user-images.githubusercontent.com/17687969/207423661-31de1535-7c75-4801-ba0e-e7f2a7e4a22e.png)

Sendo C1, 435pF, a capacitância do cabo, e a capacitância da forma de alumínio está estimada em cerca de 20pF em paralelo com C1

### Mudando estratégia: 
Formar um segundo filtro RC para cancelar os efeitos do terceiro fio sendo ligado em paralelo com o segundo fio, somando essas capacitâncias, para tal basta adicionar um segundo resistor que é acoplado quando o botão da ponta é pressionado.

Eureka! Neste ambiente de testes foi possível notar que a geometria do cabo tem um fator crucial nos valores de capacitância dos fios 2 e 3, mais precisamente, que a relação de capacitância entre eles é sempre a mesma, considerando que ambos tem exatamente o mesmo comprimento, e, a mesma distância do cabo ligado ao terra, é possível avaliar que a capacitância do fio 3 é sempre cerca de 33% do valor do fio 2, sabendo disso, basta conectar um resistor de valor 3 vezes R1, neste caso, 300KΩ, para que, de certa forma, cancele-se os efeitos de capacitância do fio 3 sobre a medição, **retirando a nescessidade do usuário de calibrar o equipamento**.

Teste feito com 100kΩ no segundo fio, como retorno de sinal do botão e 300kΩ no terceiro fio, utilizado como medida do filtro RC, o momento em que o ruído cessa é onde o botão foi apertado (peak hold), em uma superfície não condutiva, e condutiva:

<img src="https://user-images.githubusercontent.com/17687969/207409785-2a742c3c-02c4-4e33-af1e-c27619fbe15f.png" width="49%"/> <img src="https://user-images.githubusercontent.com/17687969/207409819-9821eae9-1985-470b-a928-d7fdee006ff6.png" width="49%"/>

O resultado final foi este modelo:

![image](https://user-images.githubusercontent.com/17687969/207425338-b1c99e02-5410-409e-b896-acf2a04d06f1.png)

Este modelo foi utilizado para a implementação do sistema usilizando o ESP32 como base.

Após a finalização do modelo e testes através do [ESPCORE](https://github.com/espressif/arduino-esp32) se iniciou a construção da parte física:

### PCB:

![image](https://user-images.githubusercontent.com/17687969/207434530-318e8a57-cfcb-449b-b0ed-035e6ffacba0.png)

A placa foi feita utilizando o Laboratório de Protótipos. 

![image](https://user-images.githubusercontent.com/17687969/207438199-08b3d47d-f612-48c7-a97d-778a7ff80a8e.png)

### Caixa:

A Caixa estipulada para o projeto não foi recebida por problemas de lojística, optou-se por uma outra caixa, de tamanho um pouco maior que seria possível adaptar todas as funções.

![image](https://user-images.githubusercontent.com/17687969/207439109-53cc68eb-aa85-4924-98e2-b88f0ef01b29.png)

### Montagem:

Apesar dos pontos de motagem serem diferentes do planejado, o amplo espaço da placa permitiu fazer outros furos para fixação

![image](https://user-images.githubusercontent.com/17687969/207439406-1d634d03-fa50-4ea0-9c6c-1b51dc7dbbd6.png)

## Operação:

A caixa ficou robusta, fácil de manusear, o afastamento da antena do corpo do esportista garantiu uma boa transmissão de sinal, o prendedor foi suficiente para não se soltar do corpo mesmo durante pulos.

<img src="https://user-images.githubusercontent.com/17687969/207440813-c7bb5e40-e50d-4c19-b14e-71d3c78f75f2.png" width="40%"/> <img src="https://user-images.githubusercontent.com/17687969/207440845-13f5cf0a-d4ee-4214-9250-292aadbb4e9a.png" width="40%"/>

Medição da Linha RC em ação:

<img src="https://user-images.githubusercontent.com/17687969/207599509-7af03e0b-db46-4e35-958d-6538f95ec1bd.png" width="40%"/> <img src="https://user-images.githubusercontent.com/17687969/207598078-ade5e755-1d6b-400d-9acf-0cb59786fa96.png" width="45%"/>

## Referências:

https://patents.google.com/patent/US20060100022

https://patents.google.com/patent/US20010023218A1

https://patents.google.com/patent/US3920242A
