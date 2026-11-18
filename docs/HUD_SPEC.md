# HUD e ações — especificação, sem implementação de gameplay

**Não criar botões do jogo antes de verificar os consumidores originais.** Esta entrega documenta o mapeamento público disponível; não adiciona um HUD fictício ao port.

## Referência Black estudada

`native/touch.cpp` distingue papéis por dedo: stick, look, botão, cursor, drag e pinch. IDs estáveis de controles referenciam posições normalizadas, raios, visibilidade e contextos. Há presets, escala/opacidade, edição de posição/tamanho, snapshot para Cancel, salvamento em `layout.toml` e tratamento de teclas/ponteiro junto com touch. É referência de ergonomia e organização, não fonte de semântica Urban Chaos. Os botões/fire modes/health de Black não devem ser transplantados como ações do novo título.

O novo HUD terá joystick esquerdo, arrasto de câmera/mira à direita, controles acionáveis simultaneamente, pressão contínua por duração e edição de posição/escala/opacidade. Perfis separados de celular/tablet; seleção e restauração explícitas; área segura/insets e 16:10 sem esticar a imagem. Importação/exportação somente do layout, sem misturar saves do jogo.

## Inventário público de ações Urban Chaos

Evidências: Burner `scripts/xbe_host_input.h` declara 24 slots de controle e 25 de teclado; `xbe_controls_ui.inc:7–14` nomeia os slots; `xbe_controller_bindings.inc` inicializa identidade `slot+1` e converte fontes de `input::Pad`; `xbe_keyboard_bindings.inc:37+` registra defaults e ação extra de ordens por mouse. São **slots descritos por fonte pública**, não offsets/ABI validados nesta sessão contra o XBE.

| Ação do HUD pretendida | Slot lógico público | Comportamento/evidência a validar no binário |
|---|---:|---|
| Recarregar / interagir | 0 | Original compartilha ação contextual; Burner distingue slots de teclado para prompts. Não separar artificialmente comportamento original sem consumir a função correta. |
| Arma corpo a corpo | 1 | Selecionar/ciclar; pressão prolongada tem semântica contextual. |
| Arma de fogo / troca | 2 | Equipar/ciclar; ação “anterior” é adaptação e deve ser auditada separadamente. |
| Arremessáveis | 3 | Seleção, carga e soltura dependem do título. |
| Secundária / lanterna | 4 | Contexto da arma. |
| Respirador térmico | 5 | Disponibilidade/contexto original. |
| Escudo | 6 | Segurar levanta, soltar abaixa; ataque usa ação de fogo com escudo levantado. |
| Tiro / ataque | 7 | Pressão contínua para disparo/stun/carregar arremesso conforme arma. |
| Pausa | 8 | Transição de input de gameplay para menu. |
| Objetivos | 9 | Contexto de menu. |
| Agachar | 10 | Respeitar opção original de hold/toggle. |
| Zoom / mira | 11 | Zoom suportado pela arma; não assumir semântica Black. |
| Ordem contextual | 12 | Fonte pública indica D-pad Up. |
| Menu de ordens da equipe | 13 | Fonte pública indica D-pad Down. |
| Seguir / aceitar ajuda | 14 | Contexto aliado, D-pad Left descrito nos consumidores. |
| Cobrir / recusar ajuda | 15 | Contexto aliado, D-pad Right descrito nos consumidores. |
| Movimento | 16–19 | Eixos/esquema Standard e Legacy; confirmar calibração original. |
| Câmera | 20–23 | Sensibilidade, inversão, zoom e esquema original. |
| Ordens por mouse | 24, só teclado | Adaptação PC, não controle Xbox adicional; não mapear automaticamente para touch. |

Antes de qualquer binding funcional: validar a tabela de fontes digitais/analógicas, memória do perfil, resolução por contexto, consumidores dos slots e ABI no XBE candidato. Registrar função/endereço/hash privadamente; publicar apenas conclusões e metadados permitidos. Não usar os códigos de ação de Black como equivalentes.

## Contrato de entrada proposto

Ações semânticas ficam entre UI e `input::Pad`/consumidores originais. Cada dedo tem ID e proprietário durante o gesto; mover um dedo não solta os demais. Eixos normalizados com deadzone/saturação comprovadas; estados de botões mantidos até release/cancel. Perda de foco, pausa, troca de perfil, controlador desconectado ou editor aberto liberam estados pendentes para evitar tiro/escudo travado.

Touch/gamepad Bluetooth/USB coexistem sem produzir duplicação. Android/SDL tratam controladores expostos pelo sistema; pairing não exige acesso amplo ao armazenamento. HID direto e permissões opcionais serão revistos apenas quando necessários. Não solicitar BLE indiscriminadamente.

## Layout e visual

Esquema versionado por perfil: `action_id`, posição normalizada, raio/escala, opacidade, visibilidade e âncora. Salvar atomicamente em `config/hud/`, validar números finitos/limites e migrar versões sem perder a opção de restaurar defaults. Editor com aplicar/cancelar/reset e preview do enquadramento; preview não é gameplay.

Ícones serão SVG originais, com mesma espessura/linguagem visual e estados normal, pressionado, indisponível e selecionado; escudo e ordens devem ser distinguíveis de tiro/zoom. Não foram criados ícones novos nesta entrega porque o mapeamento binário ainda não foi validado. A especificação não substitui esse gate.

## Aceitação futura

Multitouch simultâneo de mover+mirar+atirar, segurar escudo+atacar, recarregar/interagir por contexto, pressão contínua sem eventos perdidos, retorno de suspensão sem botões presos, alteração de escala/opacidade/posição persistida, perfis independentes e controle USB/Bluetooth real. Só aprovar em missão original executando, com resultados reproduzíveis.

## Evidência de execução em 2026-11-17

START (`input::Pad.buttons` máscara `0x0010`, campo digital de XINPUT_STATE em +4) abriu o menu original **PAUSED: HOPE STREET** durante a abertura da missão. A captura do frame 1920 foi inspecionada no host Linux. Isso qualifica essa observação de pausa na variante/profile atuais; não valida todos os contextos, Android ou HUD touch. A e START já navegaram pelos menus anteriores; A não foi classificado como interação em gameplay. Movimento e trigger foram agendados para frames posteriores, mas não foram alcançados pelo teste de 600 s. [Registro](evidence/runtime-progress/mission-progression.json).

## Primeiro APK experimental

A camada inicial implementa somente A/B/START e dpad para navegação, com os bits/índices de input Xbox do SDK e confirmação pelas telas originais observadas no host. Não associa botões a tiro/escudo/equipe sem comprovar seus consumidores. Joysticks/câmera por toque, editor, perfis e persistência de layout seguem pendentes; o gamepad SDL precisa de teste no dispositivo. [Primeiro teste](FIRST_TABLET_TEST.md).
