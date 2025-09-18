import re
import string

SIMBOLOS = ["M", "N", "O", "P", "Q", "R", "S", "U", "V", "X", "Y", "Z"]

def norm(s: str) -> str:
  s = s.lower()
  s = s.replace("–", "-").replace("—", "-")
  keep = "()."
  table = str.maketrans("", "", "".join(ch for ch in string.punctuation if ch not in keep))
  s = s.translate(table)
  s = re.sub(r"\s+", " ", s).strip()
  return s

def limpa_pontuacao_sentenca(s: str) -> str:
  return s.rstrip(". ").strip()

def trata_se_entao(sent: str) -> str:
  padrao = re.compile(r"\bse\s+(.*?)\s+ent(?:ão|ao)\s+(.*)$", flags=re.IGNORECASE)
  m = padrao.search(sent)
  if m:
    a = m.group(1).strip()
    b = m.group(2).strip()
    return f"({a}) -> ({b})"
  return sent

def aplica_conectivos_portugues(sent: str) -> str:
  s = sent
  s = re.sub(r"\bse e somente se\b", "<->", s)
  s = re.sub(r"\bnão\b", "~", s)
  s = re.sub(r"\s+\be\b\s+", " & ", s)
  s = re.sub(r"\s+\bou\b\s+", " | ", s)
  s = trata_se_entao(s)
  s = re.sub(r"\s+", " ", s).strip()
  return s

def separa_premissas_conclusao(texto: str):
  texto = re.sub(r"\blogo\b[:,]?\s*", " ⟹ ", texto)
  brutas = [p for p in re.split(r"[.]", texto) if p.strip()]
  partes = [limpa_pontuacao_sentenca(p) for p in brutas]

  premissas, conclusao = [], None
  for p in partes:
    if "⟹" in p:
      esq, _, dir_ = p.partition("⟹")
      if esq.strip():
        premissas.append(esq.strip())
      conclusao = dir_.strip()
    else:
      premissas.append(p.strip())

  if conclusao is None and premissas:
      conclusao = premissas[-1]
      premissas = premissas[:-1]
  return premissas, conclusao

def extrai_atomicas(formulas: list[str]) -> list[str]:
  operadores = r"(~|\&|\||<->|->|\(|\))"
  atom_set = []
  for f in formulas:
    for ped in re.split(operadores, f):
        ped = ped.strip()
        if not ped or ped in {"~", "&", "|", "->", "<->", "(", ")"}:
          continue
        if ped in {"se", "então", "e", "ou"}:
          continue
        if ped not in atom_set:
          atom_set.append(ped)
  return atom_set

def substitui_atomicas_por_simbolos(expr: str, mapa: dict[str, str]) -> str:
  itens = sorted(mapa.items(), key=lambda kv: len(kv[0]), reverse=True)
  out = expr
  for frase, simb in itens:
    out = re.sub(re.escape(frase), simb, out)
  return out

def traduzir(argumento: str):
  txt = norm(argumento)
  premissas_txt, conclusao_txt = separa_premissas_conclusao(txt)

  premissas = [aplica_conectivos_portugues(p) for p in premissas_txt]
  conclusao = aplica_conectivos_portugues(conclusao_txt) if conclusao_txt else ""

  todas = premissas + ([conclusao] if conclusao else [])
  atomicas = extrai_atomicas(todas)

  mapa = {}
  for i, a in enumerate(atomicas):
    if i >= len(SIMBOLOS):
      raise ValueError("Excedeu o repertório de símbolos disponíveis.")
    mapa[a] = SIMBOLOS[i]

  premissas_sim = [substitui_atomicas_por_simbolos(p, mapa) for p in premissas]
  conclusao_sim = substitui_atomicas_por_simbolos(conclusao, mapa) if conclusao else ""

  return premissas_sim, conclusao_sim, mapa

texto = input("Digite um argumento em português:\n")

prem, conc, mapa = traduzir(texto)

print("\nForma simbólica:")
if prem:
  print("Premissas:")
  for i, p in enumerate(prem, 1):
    print(f"  ({i}) {p}")
if conc:
  print("Conclusão:")
  print(f"  ⊢ {conc}")

print("\nMapa de proposições:")
for frase, simb in mapa.items():
  print(f"  {simb} = {frase}")
