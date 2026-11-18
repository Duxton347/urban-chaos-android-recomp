# Tela e controles — versão experimental 0.3

O HUD de controle substitui a barra inferior por círculos translúcidos sobre a superfície SDL inteira. Não representa vida, munição ou objetivos: o HUD original depende da missão ainda presa no loading. A captura enviada pelo proprietário confirma perfil, menu e tela Hope Street, não gameplay. O código continua ligado ao mesmo runtime original otimizado.

## Uso no tablet ou celular

O botão **⋯**, no canto superior direito, abre os ajustes e a exportação do relatório. Não ocupa uma faixa permanente da imagem.

- **Navegação de menus:** direcionais, A selecionar, B voltar, X apagar perfil quando o jogo indicar, START e BACK do controle Xbox. BACK não substitui B.
- **Controle Xbox completo:** acrescenta os dois analógicos, Y, gatilhos LT/RT, Black/White e cliques L3/R3. Analógicos têm deadzone radial, saturação e retorno ao centro; botões ficam pressionados até soltar/cancelar. Cada dedo mantém seu controle sem soltar os demais. Não há nomes de ações Urban Chaos inventados: o esquema original do jogo determina as funções de cada fonte Xbox.
- **Perfil celular/tablet:** presets independentes, inicialmente escolhidos pela largura mínima Android. Cada combinação de perfil e modo tem seu layout.
- **Ocultar controles:** permite usar o gamepad deixando somente ⋯; ocultação vale para a sessão. Bluetooth/USB e controles em missão ainda exigem teste real.

Para editar, selecionar **Editar HUD**, tocar e arrastar um controle e abrir ⋯ para alterar seu tamanho ou opacidade. **Salvar layout** grava as mudanças; **Cancelar edição** restaura o snapshot anterior; **Restaurar posições padrão** pode ser confirmado salvando ou desfeito cancelando. Durante a edição não se enviam comandos touch ao jogo. Pausar pelo START antes de editar em gameplay: o editor não injeta uma pausa sintética.

Geometria normalizada, tamanho e opacidade são salvos com `AtomicFile` em `files/config/hud/{phone,tablet}-{menu,xbox}.json`; seleção de perfil/modo/enquadramento em `settings.json`. Schema 1, máximo 64 KiB, IDs fixos e únicos, rejeição de valores não finitos/fora dos limites e escrita atômica. O arquivo inválido não ativa parcialmente um layout. Clamping mantém controles na área segura ao trocar dimensões; nenhuma configuração mistura-se com os saves. A restauração física de um arquivo gravado e a ergonomia no Pad 6 ainda precisam do novo teste manual.

## Enquadramento

**Ajustar** é o padrão: mostra a imagem inteira mantendo a proporção original. **Preencher** cobre a tela mantendo a proporção e recorta bordas. **Esticar** cobre a tela deformando as proporções, identificado explicitamente no seletor. A escolha é persistida e atualizada na apresentação Vulkan sem recompilar shaders ou alterar os recursos.

O retângulo acompanha o tamanho real do swapchain. O host usa fullscreen imersivo; o overlay respeita insets de barras/cutout. São opções de apresentação, não uma implementação de câmera 16:10 ou aumento automático da resolução interna. O FOV e o enquadramento original não foram reescritos. Em 1280×800, a imagem 4:3 inteira ocupa aproximadamente 1066,67×800; o modo preencher usa 1280×960 com recorte vertical de 80 pixels por lado. Interfaces originais podem ficar fora da tela nesse modo; Ajustar recupera a imagem completa.

## Toques curtos e atraso

Antes, um toque completo entre duas chamadas guest desaparecia: o JNI só mantinha o último booleano. Agora um press curto fica pendente até uma consulta `XInputGetState` recebê-lo. Solturas e pressão contínua são mantidas; toques excedentes são coalescidos em vez de formar uma fila longa num jogo lento. Limpar foco/pausa/editor solta controles e descarta presses pendentes.

A auditoria encontrou também `xdk/xapi.cpp::UpdatePorts`: a thread de detecção de controle consultava `input::Poll` a cada 250 ms. No Android isso poderia consumir o press pendente antes de `XInputGetState`. O patch MIT explícito `android-input-sampling.patch` consulta somente `input::Connected`, preservando os presses para a leitura de estado. Outros hosts continuam com o comportamento original. O teste de integração chama o host real e verifica vinte consultas de conexão antes da leitura do press.

`UC-INPUT-001`, já existente no coletor Java, registra perfil/modo/enquadramento e resultado de salvar o layout. O novo **`UC-INPUT-002`** nativo mede presses recebidos, entregas, tempo acumulado e maior tempo entre JNI e consumo pelo guest. Não mede latência completa até os pixels da tela. A resposta visual continua limitada pela execução/renderização: esta alteração não promete eliminar atraso quando o jogo roda lentamente. Nenhum toque individual, coordenada ou arquivo de save é exportado.

## Validação executada e limites

Passaram 78 checks C++ de fontes Xbox, taps entre polls, holds, liberação, coalescência, eixos e viewport; integração real do host de input; 16 checks Java de roteamento multitouch/layout; 326 checks Java de diagnósticos; 14 de cópia e testes JNI/ZIP. A Activity/SDL/HUD compilou contra SDK34/Java17. Renderer autoral Vulkan e contratos AArch64/superfície Android passaram. Biblioteca completa ARM64 e APK foram construídos e verificados.

A primeira fixture de viewport exigia igualdade exata de floats; foi corrigida para tolerância de 0,01 pixel, sem mudar o cálculo produtivo. O primeiro teste agregado posterior atingiu a margem mínima de espaço do importador no workspace da nuvem e falhou; intermediários regeneráveis de um build Android antigo/falho foram removidos, preservando entradas e APKs entregues, e a reexecução passou sem relaxar a proteção. Logs das tentativas permanecem privados.

Esses resultados não substituem instalação e toque no dispositivo. Próximo ensaio: instalar por cima, navegar em A/B/X/direcionais, verificar holds, alternar enquadramento, salvar/cancelar e reabrir o layout, alternar perfil, suspender/retomar sem controle preso e comparar a nova métrica de input. Se Hope Street continuar no loading, aguardar 60 segundos e exportar pelo ⋯. A missão, comandos semânticos de escudo/equipe, HUD original e fidelidade continuam sem qualificação.

## Artefato privado

Versão 0.3, `experimental-40f8853d42ba3884`, pacote `org.urbanchaos.launcher`, versionCode 3, mesma assinatura debug da primeira instalação. APK: 28.202.850 bytes; SHA-256 `4340ecd6205b63cce6005936b50be4d1c2737d75f79b0b971232ef45c8db4411`. Sem ISO/XBE/recursos e sem distribuição pública do código derivado. [Recibo sanitizado](evidence/runtime-progress/android-touch-hud.json). Não desinstalar ou limpar dados para atualizar.
