# Testes manuais no Pad 6

**PENDENTE.** Nenhum item desta página foi executado no tablet nesta sessão. Não há ADB presumido nem acesso ao computador pessoal. O APK é diagnóstico; não executa Urban Chaos.

## Identificação e instalação

1. Registrar modelo exato, versão Android, RAM, versão de sistema, modo de energia e estado térmico.
2. Conferir SHA-256 do APK em `artifacts/SHA256SUMS`. Transferir pelo recurso de arquivos do cliente e abrir no instalador Android. Autorizar a origem de instalação quando o sistema solicitar.
3. Verificar nome **Urban Chaos · Diagnóstico**, versão `0.1-diagnostic`, pacote `org.urbanchaos.diagnostic`. Requer Android 10/API 29 ou superior e ARM64. O Pad 6 real ainda precisa ser identificado.
4. Abrir e registrar sucesso/falha, sem inferir sucesso da compilação. Se não houver Vulkan adequado, o teste deve apresentar erro em vez de declarar sucesso.

## Vulkan e lifecycle

1. Abrir “Testar Vulkan / SDL3”. Confirmar cores variando continuamente, sem tela preta ou congelamento; esses frames são sintéticos.
2. Alternar apps, apagar/acender tela, voltar ao diagnóstico e girar o dispositivo. Repetir dez vezes; registrar falhas, recriação de superfície e duração.
3. Encerrar o teste, abrir novamente e exportar relatório pelo menu principal. Ele inclui modelo/Android e `logs/vulkan.log`: GPU, driver, API Vulkan selecionada, dimensões de swapchain, suspensão/resumo e erros.
4. Anexar observações reais ao relatório; os logs não comprovam que a imagem foi visualmente apresentada. A enumeração atual também não é um inventário completo de extensões/limites Vulkan.

## Importação SAF

1. Testar arquivo PS2: esperar mensagem específica e ausência de ativação. Usar “Descartar importação incompleta” para liberar staging e escolher outra origem.
2. Selecionar pasta Xbox pelo seletor do sistema ou arquivo XBE/XISO/XDVDFS. Não é solicitada permissão ampla de armazenamento. O arquivo XBE precisa ter o hash candidato; possuir somente XBE não fornece os demais assets necessários ao futuro jogo.
3. Durante cópia longa: pausar, retomar, encerrar o processo pelo Android e reabrir/retomar. Verificar progresso. Ao retomar uma cópia, o prefixo existente é comparado byte a byte antes de continuar.
4. Testar espaço insuficiente, arquivo truncado, hash diferente e perda de permissão do provedor SAF. A instalação anterior deve permanecer ativa. Arquivos sem tamanho informado pelo provedor são recusados.
5. Na extração XISO, cancelar e retomar. A imagem já copiada é reutilizada e verificada; a fase de extração recomeça, preservando o staging incompleto até terminar.
6. Confirmar que somente a importação validada é ativada via `game/active.json`. Uma interrupção entre renomear staging e gravar o ponteiro pode ser recuperada pelo journal. Testar atualização do APK com a mesma assinatura preservando `game`, `saves` e `config`.

Não há validação do conjunto completo dos assets do disco nem de cada arquivo contra um manifesto comercial nesta versão: o gate de ativação confirma o candidato XBE e a integridade estrutural/limites da extração. A sessão de importação está no armazenamento privado e nunca integra o APK.

## HUD e entrada

Arrastar botões em modo EDITAR. Alternar para TESTAR e pressionar múltiplos botões simultaneamente; verificar indicações independentes e liberação ao cancelar/alternar apps. Fechar/reabrir para conferir layout persistido. Testar gamepad Android: A/primary, B/secondary, X/interact, Start/pause e eixos move/look. São identificadores diagnósticos; nenhuma ação foi vinculada aos consumidores do Urban Chaos.

## Testes do jogo — bloqueados

ABI/memória/callbacks, logo/menu, missão completa, áudio, checkpoint/morte/reload, cutscenes, saves após encerramento, update preservando saves do jogo, multitouch em gameplay, aspecto 16:10 e sessões térmicas de 15–30 minutos só podem ser registrados depois dos gates correspondentes. Nenhum desses resultados foi obtido pelo diagnóstico.

## Registro a preencher

| Campo | Resultado real |
|---|---|
| Modelo e Android | Pendente |
| GPU / driver / API e capacidades Vulkan | Pendente |
| Instala e abre | Pendente |
| Frames sintéticos e lifecycle | Pendente |
| SAF: interrupção e retomada | Pendente |
| Multitouch/gamepad/HUD persistido | Pendente |
| Atualização com mesma assinatura preserva dados | Pendente |
| Urban Chaos: menu / missão / campanha | BLOCKED |
