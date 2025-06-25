#  *sqsgen* under the hood

## Pair Short-Range-Order

### What does *sqsgen* actually compute?

*sqsgenerator* computes so-called *Warren-Cowley* short-range-order (WC-SRO) parameters, to quantify the "*randomness*" of an atomistic model. Thus consider a binary alloy with just $A$ and $B$ atoms, then the  SRO parameter is defined as

```{math}
:label: eqn:wc-sro
\alpha_{AB} = 1 - \dfrac{N_{AB}}{NMx_Ax_B}
```

Where $N_{AB}$ the number of $A-B$ bonds, $N$  the number of atoms in the cell, $x_A$ ($x_B$) the mole fraction of $A$ ($B$) and $M$ the number of neighbors on the lattice (e. g $M^{fcc}=12$). Thus it compares the number of $A-B$ bonds in a given (numerator) configuration with that of statistical ideal solid solution (denominator). Thus the following cases can occur

* $\alpha_{AB} < 0$: system tends to be **ordered**
* $\alpha_{AB} > 0$: system tends to be **clustered**
* $\alpha_{AB} \approx 0$: system tends to be **disordered**

Therefore, *sqsgenerator* tries to optimize a given system such that the SRO approaches zero.

### Can *sqsgenerator* handle also second-nearest neighbors?

Yes it actually can! Indeed, on a *fcc* lattice each atom exhibits $M^{fcc}_1=12$ nearest neighbors, but there are also  $M^{fcc}_2=6$ second nearest neighbors and even  $M^{fcc}_3=24$ third nearest neighbors. However, it is very easy to generalize the *Warren-Cowley* SRO to more "*coordination shells*" which, therefore becomes a vector

```{math}
:label: eqn:wc-sro-shells
\alpha^k_{AB} = 1 - \dfrac{N_{AB}^k}{NM^kx_Ax_B}.
```

### But what about optimizing multi-component alloys?

Similarly, the generalization to multi-component systems is a straight as for the coordination shells. Consider a system of $\{A,B,C\}$ atoms. Now we have to also consider $A-C$ and $B-C$ bonds, thus resulting in 6 SRO parameters for each coordination shells (also $A-A$, $B-B$ and $C-C$ bonds are considered), thus the initial parameter becomes a three dimensional object

```{math}
:label: eqn:wc-sro-multi
\alpha^i_{\xi\eta} = 1 - \dfrac{N^i_{\xi\eta}}{NM^ix_{\xi}x_{\eta}}
```

where again $M^{i}$ is the number of sites in the $i$-th coordination shell, $x_\xi$ and $x_\eta$ are the mole-fraction of the corresponding species and $N^i_{\xi\eta}$ the number of $\xi-\eta$ "*bonds*"/pairs in the coordination shell.

### Putting it all together!

This is a quite large number of parameters to optimize. In order to squeeze those together we produce a single scalar value by simply summing over those parameters. Therefore, we define our "*objective-function*" as for a certain atomic configuration $\sigma$ as,

```{math}
:label: eqn:objective
\mathcal{O}(\sigma) = \sum_{k}w^i\sum_{\xi,\eta} p_{\xi\eta} | \tilde{\alpha}_{\xi\eta}-\alpha^i_{\xi\eta} |
```

where $w^i$ are the coordination *{ref}`shell_weights <input-param-shell-weights>`*, to take into account the decreasing influence of shells which are farther away.  $\alpha_{\xi\eta}$ are the *{ref}`target_objective <input-param-target-objective>`* parameters. Those can be used to optimize pairs between different species differently.
$p_{\xi\eta}$ are *{ref}`pair_weights <input-param-pair-weights>`* and allows the user to weight pair types differently.

However, for the sake of computational efficiency we merge $p_{\xi\eta}$ and $w^i$ into $\tilde{p}_{\xi\eta}^i$ where

```{math}
:label: eqn:parameter-weight-efficient
\tilde{p}_{\xi\eta}^i = w^i p_{\xi\eta}
```

Moreover, the $\dfrac{1}{NM^i x_\xi x_\eta}$ term does not depend on a specific configuration $\sigma$ therefore it can
be computed before the actual optimization process. Hence, we gather those and name them "*{ref}`prefactors <input-param-prefactors>`*" $f_{\xi\eta}^i$
where

```{math}
:label: eqn:prefactors
f_{\xi\eta}^i = (NM^ix_\xi x_\eta)^{-1}
```

such that Eq. {eq}`eqn:wc-sro-multi` can be factored to

```{math}
:label: eqn:sro-modified
\alpha^i_{\xi\eta} = 1 - f_{\xi\eta}^i N^i_{\xi\eta}
```


Employing the above definition the objective function $\mathcal{O}(\sigma)$ can be written as

```{math}
:label: eqn:objective-actual
\mathcal{O}(\sigma) &= \sum_{i,\xi,\eta} \tilde{p}_{\xi\eta}^i | \tilde{\alpha}_{\xi\eta}-\alpha^i_{\xi\eta} |  \\
 &=\sum_{i,\xi,\eta} \underbrace{\tilde{p}_{\xi\eta}^i}_{\texttt{pair_weights}} | \overbrace{\tilde{\alpha}_{\xi\eta}}^{\texttt{target_objective}}- \overbrace{(1 - \underbrace{f^i_{\xi\eta}}_{\texttt{prefactors}}N^i_{\xi\eta})}^{\alpha^i_{\xi\eta}} |
```

*sqsgenerator* tries different atomic configurations $\sigma$ by "*randomly shuffling*" or "*systematically*" (in lexicographical order) to filter out those configuration which **minimize** $\mathcal{O}(\sigma)$.

```{note}
In the technical part of the documentation we refer to $w^i$ as the
*{ref}`shell_weights <input-param-shell-weights>`*,
$\tilde{\alpha}_{\xi\eta}$  as the *{ref}`target_objective <input-param-target-objective>`*, $\tilde{p}^i_{\xi\eta}$ as the *{ref}`pair_weights <input-param-pair-weights>`* and $f_{\xi\eta}^i$ as the
*{ref}`prefactors <input-param-prefactors>`*

```

```{note}
The mapping of an atomic configuration $\sigma$ on a scalar values via $\mathcal{O}(\sigma)$ is non-injective, thus you might end up with quite a lot of configurations, which are "*equal*" in the here presented formalism.
```
