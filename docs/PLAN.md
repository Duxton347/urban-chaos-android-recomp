# Execução por gates

> Histórico da etapa anterior. A rota vigente está em [BUILD_STATUS.md](BUILD_STATUS.md). O diagnóstico não faz parte da entrega do jogo; o teste determinístico antigo não comprovava funções emitidas, corrigido na nova rota.

Plano original preservado em [REQUESTED_PLAN.md](REQUESTED_PLAN.md). Esta execução substitui a antiga condição “sessão em planejamento” por implementação e evidências reais dos componentes independentes.

| Gate/fase | Status | Evidência / condição de saída |
|---|---|---|
| F0 — fontes e licenças | GO para auditoria; BLOCKED para adaptações Burner | Três projetos fixos, lockfile, consumidores, licenças e comparação produzidos. Não há conclusão de clean build do título. |
| Infra host | GO | Scanner/recompilador Linux compilados isoladamente; testes sintéticos. |
| Diagnóstico Android | GO para APK debug | SDL3/Vulkan, SAF com staging/retomada, HUD diagnóstico. Instalação, apresentação de frames e lifecycle em hardware pendentes. |
| G1 — geração original | BLOCKED; análise de símbolos será REQUIRES_REVERSE_ENGINEERING | XBE extraído, duas gerações idênticas e compilação PC concluídas; revisão HLE/ABI e qualificação pendentes. Execução Linux passou primeira falha DSOUND com patch experimental e falha em D3D antes de menu. Logs reais, configuração completa, ABI e geração limpa determinística são obrigatórios. |
| G2 — boot ARM64 | BLOCKED por G1 | Código real gerado compilado, memória, kernel, threads, arquivos e callbacks testados. |
| G3 — menu/recursos | BLOCKED por G1–G2 | D3D8→Vulkan e áudio do Urban Chaos; frames reais de logo/menu. Cores do diagnóstico não satisfazem esse gate. |
| G4 — missão/saves | BLOCKED por G3 | Missão original, checkpoint, morte, reload, cutscenes, save/load após encerramento. |
| G5 — controles/HUD | BLOCKED para jogo; infraestrutura independente GO | Touch, gamepad, layout persistido e enquadramento 16:10 validados com consumidores originais. Ações atuais são apenas diagnóstico. |
| G6 — desempenho | BLOCKED por missão | Pad 6 identificado; medições reais de 15–30 minutos; opções gráficas reversíveis somente então. |
| G7 — entrega do jogo | BLOCKED | APK de jogo só após gates. Release requer keystore do proprietário. Apenas diagnóstico debug entregue agora. |

## Escala de status

| Produto | Compila | Inicia | Menu funciona | Missão jogável | Campanha verificada |
|---|---|---|---|---|---|
| Diagnóstico | Sim | Não testado no dispositivo | Não se aplica ao jogo | Não se aplica | Não se aplica |
| Urban Chaos Android | Não demonstrado | Não demonstrado | Não demonstrado | Não demonstrado | Não demonstrado |

## Próxima ação verificável

Testar manualmente o APK diagnóstico pelo roteiro [DEVICE_TESTS.md](DEVICE_TESTS.md). O executável Xbox já está disponível privadamente. A rota atual é corrigir a inicialização gráfica identificada após o patch DSOUND experimental na [inspeção real](USER_IMAGE_INSPECTION.md), revisar a variante e validar boot/menu PC antes de ARM64. A importação não equivale à aprovação de G1. Não foram criados hooks/endereços Urban Chaos nem status de qualificação fictícios.
