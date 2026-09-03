# Simulador de Gravidade 3D

Simulador interativo de física gravitacional e deformação do espaço-tempo desenvolvido em C++17 e OpenGL 3.3.

## Funcionalidades
* **Física Multicorpo:** Simulação de atração gravitacional e colisão entre múltiplas esferas em tempo real.
* **Malha Espaço-Tempo:** Deformação do plano 3D baseada na massa e distância dos corpos.
* **Interação Dinâmica:**
  * **Clique Esquerdo:** Gera novas esferas com direções e cores aleatórias.
  * **Clique Direito:** Aumenta a massa do objeto em criação.
* **Navegação Livre:** Câmera 3D estilo *flycam* com velocidade ajustável e zoom.

## Controles
| Comando | Ação |
| :--- | :--- |
| **W, A, S, D** | Movimentar a câmera |
| **ESPAÇO / CTRL** | Elevar ou baixar a altura |
| **SHIFT (Segurar)** | Aumentar velocidade da câmera |
| **Scroll do Mouse** | Aproximar / Afastar zoom |
| **Clique Esquerdo** | Spawnar nova esfera |
| **Clique Direito** | Aumentar massa da esfera |
| **K** | Pausar a simulação |
| **Q** | Fechar aplicação |
