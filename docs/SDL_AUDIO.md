# Saída estéreo SDL3 compartilhada

O pin do SDK recebe do mixer **float intercalado L/R, 48 kHz, dois canais** em audio::StartOutput. O Black usa outro contrato, de seis canais. Sua dobra 5.1 não foi transplantada: native/audio/sdl_output.* implementa uma saída própria para o contrato do SDK usado por Urban Chaos, com SDL release-3.4.16 fixado no lockfile.

## Implementação

SDL_OpenAudioDeviceStream recebe F32/2/48000; o SDL converte para o dispositivo físico quando necessário. O callback solicita o mixer original em blocos de até 480 frames e envia os mesmos canais ao stream. Não há fold, troca de canais ou resampling adicional próprio. Pedidos de bytes são arredondados para frames completos, com orçamento máximo de 80 ms por chamada. Valores não finitos são silenciados e contados; valores normais não são alterados.

O buffer é pré-alocado. O código do callback não grava logs/arquivos e não destrói o stream. Falha de render/submissão muda o estado para Failed; a coleta periódica no host registra o código. Métricas contam frames submetidos, e não comprovam reprodução audível ou fidelidade de músicas/vozes/efeitos.

SetOutputPaused pausa/retoma o dispositivo e impede novas solicitações ao mixer após o retorno da pausa. StopOutput destrói o stream antes de liberar o callable, esperando callbacks ativos. Cada inicialização SDL é balanceada com QuitSubSystem; não se encerra o SDL inteiro pertencente ao host. O estado de pausa anterior à criação do stream é preservado.

Isso **não pausa o jogo**. Na futura Activity/loop SDL, esses métodos precisam ser coordenados com as threads guest e a superfície Vulkan. Não substituir essa coordenação por um botão START sintético ou por hooks de Black. A política de pausa e a integração Android completa continuam pendentes.

## Diagnósticos

UC-AUDIO-001 mantém métricas do mixer/transporte. UC-AUDIO-002 indica falha de operação SDL: 1 inicialização, 2 abertura, 3 retomada, 4 pausa, 5 submissão e 6 callback do mixer. A causa física do driver permanece não classificada. UC-AUDIO-003 registra estado: 0 parado, 1 pausado, 2 executando, 3 falhou. O observer grava os códigos e números; não copia strings arbitrárias do driver ou conteúdo sonoro.

## Build e execução de pesquisa

1. Preparar dependências públicas fixas: python3 scripts/ci_bootstrap.py --component host.
2. Compilar SDL e executar as fixtures: python3 scripts/build_sdl_audio.py.
3. Usar scripts/build_research_runtime.py com as entradas privadas já geradas, o perfil correto, --audio-output sdl e --sdl-build build/sdl-audio/sdl.
4. No gravador privado, --audio-driver dummy seleciona explicitamente o dispositivo silencioso de teste. O recibo registra essa seleção e audible_output_verified=false. O modo measure continua disponível para comparação; não há fallback silencioso automático de SDL para esse sink.

O host é Linux e usa o renderer de referência; esta configuração SDL sem janelas usa SDL_UNIX_CONSOLE_BUILD. A primeira configuração falhou pela verificação de X11/Wayland, corrigida explicitamente para esse host somente de áudio. O build Android final terá sua própria janela/superfície, não essa configuração headless.

## Testes e limites

As fixtures usam sinais de autoria própria e SDL real com driver dummy. Um postmix observa L/R na saída, verifica ordem, razão entre canais, valores finitos e frequência. Também são testados pausa antes de iniciar, retomada, encerramento aguardando callback bloqueado, reinício, exceção do render, NaN e driver inexistente. Não gravam PCM ou sons do jogo. A compilação ARM64 inclui o mesmo sdl_output.cpp contra os headers SDL fixados, sem alegação de execução no dispositivo.

O host com o mixer original foi ligado a essa saída para a regressão privada documentada em BUILD_STATUS.md. O teste dummy comprova entrega de amostras ao SDL, não que alguém ouviu música ou que todos os comandos DirectSound do jogo estão corretos. APIs ainda executadas como código Xbox original continuam sob auditoria. Vulkan com reports reais, missão completa, saves, áudio audível e suspensão no Pad 6 exigem testes posteriores.

## Correção do encerramento do host

O primeiro ensaio real exigiu SIGKILL: SDL instalava um handler de SIGTERM que enfileirava quit, enquanto o host Linux de pesquisa não processava eventos SDL. KeepHeadlessSignalHandlers define SDL_HINT_NO_SIGNAL_HANDLERS antes da inicialização do áudio, mantendo o controle de sinais no runner. A política não está no backend compartilhado e não altera a política Android. O teste autoral confirma o comportamento antigo e SIGTERM/-15 com a correção; a regressão do jogo também encerrou sem kill forçado. O limite de tempo continua sendo registrado como limite, não crash ou saída normal do jogo.
