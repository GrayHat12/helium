$$
\begin{align}
    [\text{Prog}] &\to [\text{Stmt}]^* \\
    [\text{Scope}] &\to [\{\text{Stmt}]^*\} \\
    [\text{Else}] &\to 
    \begin{cases}
        [\text{If}] \\
        [\text{Scope}] \\
        \epsilon \\
    \end{cases} \\
    [\text{Let}] &\to 
    \begin{cases}
        let\space\text{ident} = [\text{Expr}]; \\
        let\space \text{mut}\space\text{ident} = [\text{Expr}]; \\
    \end{cases} \\
    [\text{If}] &\to \text{if}\space[\text{Expr}][\text{Scope}]\space[Else] \\
    [\text{Stmt}] &\to 
    \begin{cases} 
    exit([\text{Expr}]); \\
    [\text{Let}] \\
    [\text{Scope}] \\
    [\text{If}] \\
    \text{ident}=[\text{Expr}];
    \end{cases} \\
    [\text{Expr}] &\to 
    \begin{cases}
        [\text{Term}] \\
        [\text{Operation}] \\
    \end{cases} \\
    [\text{Operation}] &\to [\text{Expr}]\space[\text{Operator}]\space[\text{Expr}] \\
    [\text{Operator}] &\to 
    \begin{cases}
        \text{+} & \text{prec}=0 \\
        \text{-} & \text{prec}=0 \\
        \text{/} & \text{prec}=1 \\
        \text{*} & \text{prec}=1 \\
        \text{\%} & \text{prec}=0 \\
    \end{cases} \\
    [\text{Term}] &\to
    \begin{cases}
        \text{int-lit} \\
        \text{ident} \\
        \text{([Expr])} \\
    \end{cases}
\end{align}
$$