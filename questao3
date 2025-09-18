from sympy import sympify, satisfiable

def verificar_satisfatibilidade():
  
  sentenca = input("""Digite uma sentença da lógica proposicional:
OPERADORES:
 & → E (conjunção)
 | → OU (disjunção)
 ~ → Negação
 >> → Implicação
 << → Implicação reversa
""")

  try:
    expr = sympify(sentenca)
      
    modelo = satisfiable(expr)

    if modelo == False:
      print("A sentença NÃO é satisfatível (é uma contradição).")
      
    else:
      print("A sentença é satisfatível!")
      print("Um modelo de atribuição que a torna verdadeira é:")
      print(modelo)

  except Exception as e:
    print("Erro ao processar a sentença. Verifique a sintaxe.")
    print(e)

verificar_satisfatibilidade()
