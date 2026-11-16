# Retornos não locais na recompilação estática

Decisão experimental de 2026-11-16. Base MIT XboxXBERecomp `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7`; alteração explícita em `patches/runtime/resumable-context.patch`. O checkout pinado não é modificado. O tool usa uma cópia só dos fontes/headers CPU necessários; patches de áudio e GPU são aplicados separadamente ao host. A opção está desativada por padrão e não qualifica G1.

## Falha reproduzida

Durante o carregamento da primeira missão, a rotina original `0x002218C0` restaura registradores, troca ESP por outra pilha e retorna ao PC salvo. O compilador original transforma RET em incremento de ESP seguido de `return` C++. Esse retorno segue a pilha nativa, mesmo quando o endereço e a pilha guest mudaram. A corrupção posterior apareceu em `0x002012C0`: um endereço dentro de código foi tratado como nó de lista. Os recursos solicitados estavam disponíveis; copiar mais assets não corrigiria esse comportamento.

Não foi removida a rotina original de contexto, ignorada a lista ou inserido retorno de sucesso. A mudança modela o retorno guest real.

## Implementação optativa

Com `[xbe] resumable=true`, o gerador registra continuações após chamadas e verifica o endereço e o slot de pilha de cada RET. Um retorno incompatível com a chamada C++ lança uma transferência que desenrola os frames nativos. O dispatcher retoma o C++ já recompilado no PC guest correspondente e acompanha os retornos restantes. Não há interpretação de instruções comerciais, recompilação dinâmica ou emulação de PS2.

Campos adicionais do contexto são de propriedade do runtime: PC de retomada, destino/slot de retorno e profundidade do dispatcher. As rotinas HLE registram seus retornos; os chamadores nativos continuam usando `kernel::CallGuest`.

A primeira versão propagava todas as transferências de callbacks para o dispatcher externo. O jogo revelou um retorno sentinela pertencente ao callback, encerrando o teste antes do menu. A revisão permite que cada invocação resolva suas transferências locais e propague retornos pertencentes a outra invocação. Retorno sem correspondência e continuação desconhecida continuam falhas explícitas. Callbacks que abandonam operações nativas com recursos ou locks ativos exigem revisão própria; esses casos não estão globalmente qualificados.

Hooks por instrução são rejeitados neste modo enquanto suas saídas não tiverem modelagem compatível. A configuração usada nesta investigação não contém esses hooks. O modo não corrige automaticamente limites de funções, ABI, SEH, jump tables ou APIs ausentes.

## Regressão sem dados comerciais

```bash
scripts/build_host.sh
python3 scripts/build_context_tools.py
python3 scripts/context_smoke.py
```

`tests/fixtures/context_switch.s` é código autoral: preserva registradores, salva contexto, alterna entre duas pilhas e deve retornar 42. O driver usa memória e dispatcher reais do SDK. Não contém bytes, funções ou fixtures do jogo.

| Teste executado | Resultado |
|---|---|
| Compilador/dispatcher originais, controle negativo | 2989, falha esperada |
| Continuação corrigida | 42, ESP e ESI preservados |
| Mesmo cenário dentro de callback nativo aninhado | 42, ESP e ESI preservados |
| Duas gerações com continuações | Idênticas |
| Geração C++ com opção desativada | Idêntica à original |
| Referência x86 independente do fixture | 42 |

A execução i386 direta não é suportada neste ambiente Linux (`Exec format error`). A referência local usou Unicorn 2.1.4, pin/checksum em `dependencies.lock.json`, exclusivamente no fixture autoral. Ele não executou o jogo e não é dependência do port. No CI o script tenta execução i386 nativa; se também não houver ferramenta opcional, informa referência indisponível, sem inventar validação independente.

## Aplicação privada ao título

O XBE fornecido gerou 29.049 funções e 83.240 entradas de continuação; duas gerações produziram 132 arquivos / 227.434.775 bytes idênticos. As 129 unidades (128 partições + mapping) compilaram no Linux. O host foi recompilado com o mesmo layout de contexto e macro `XBE_RESUMABLE_GUEST=1`.

As saídas e os logs do título permanecem em diretórios ignorados. [Progresso do runtime](RUNTIME_PROGRESS.md) registra a execução posterior. Esses números não significam missão jogável ou compatibilidade Android.
