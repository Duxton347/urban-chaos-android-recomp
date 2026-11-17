# Reaproveitamento PC → Android

Referências fixas: Burner `bdaef722ef1d589af30e5a58dd8543f64564c4fc`, SDK MIT `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7`. A Burner usa o mesmo pin do SDK desta pesquisa; seu Linux é Windows/Proton, não o host Linux nativo que estamos construindo. O pacote PC não fornece a geração qualificada completa. [Auditoria e tentativa executada](PC_BUILD_RECOVERY.md).

| Componente inspecionado | Aproveitamento e condição |
|---|---|
| `prepare_xbe_resource_refs.py` | Pista aplicada: os três getters devem reter o pai pela primeira referência pública. Contrato conferido no XBE e implementação independente chamando a rotina original; [evidência](RESOURCE_LIFETIME.md) |
| `dependencies.lock.json` da Burner | Identifica backports upstream por commit; revisar o diff original MIT sem importar toda a árvore ou seus hooks |
| SDK: `5f760a2714cb1abe11b8620354246d0a659ac132` | Consulta direta e EEPROM devem concordar sobre configuração estéreo; correção upstream de duas constantes, candidata a teste independente |
| SDK: `c9cda4ac39e1e8823a1517f950966a7ee1bd3905` | Estágio fixed-function que lê textura ausente preserva a cor corrente. Exige regressão de combinações e efeito no renderer, não descarte global de texturas |
| SDK: `1bb872b181c40de83ca2f69d9cd45b442a362d1d` | Commit de áudio amplo (quatro arquivos); o PC seleciona somente cache de dois blocos ADPCM com invalidação adicional. Não aplicar integralmente ao mixer atual |
| SDK: `908dc53a40d4544ab8900d3d9eb3bda174daf846`, `0fd00442660573dc342b41c11c6574318f0e8698` | Otimizações D3D12 de uploads/bindings. Candidatas posteriores; ownership e invalidação são contratos úteis para Vulkan, implementação D3D12 não é backend Android |
| `docs/XBE-MENUS-AND-INPUT.md`, `CODE_MAP.md` | Documentam snapshot de input por jogador, contextos de menu e consumidores originais. Conferir cada seam contra o hash do XBE; não incorporar menus e extras PC como lógica original |
| Seção recorded/inline draws em `XBE-RUNTIME-MAINTENANCE.md` | Preservar ordem, estado, ownership dos índices pela fila e rejeição de pacotes inválidos. Não cachear conteúdo de packets; otimização só depois da correção funcional |
| `xbe_audio_dsp.inc`, configuração e roteamento | Referências para DSP, sends e filtros ainda não qualificados. Métricas do mixer não comprovam música correta nem áudio ouvido |
| Scripts próprios e fixtures privados da Burner | Licença geral das adaptações não esclarecida; não incorporados. Casos e resultados descritos orientam investigação independente, sem copiar a implementação |
| Alterações de corpo-escudo, co-op, cheats e texturas | Fora do objetivo de preservar o jogo original; não usar como requisito da primeira missão |

## Menor caminho verificado

Manter geração e runtime atuais; corrigir o ciclo de vida da memória observado na carga; apresentar a cena original; executar os comandos reais com snapshots reproduzíveis; validar áudio e checkpoint. O arquivo AArch64 já compilado não substitui essas etapas. Aplicar backports pequenos com testes próprios e manter a base pinada reduz o custo de revisão sem introduzir diferenças de ABI desconhecidas.

Não se comprovou equivalência da variante do usuário ao executável de referência Burner. Seus endereços são pistas para inspeção, não bindings automaticamente válidos. Não há evidência de que copiar o pacote Windows elimine o defeito de memória observado no host atual.
