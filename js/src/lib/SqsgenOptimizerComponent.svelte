<script lang="ts">
    import {
        type Content,
        JSONEditor,
        type TextContent,
        Mode,
        type MenuButton,
        type MenuSeparator,
        type MenuItem,
        type RenderMenuContext,
        type OnChangeStatus,
        type ValidationError
    } from 'svelte-jsoneditor'
    import {loadOptimizer, SqsgenOptimizer} from '$lib/optimizer.js'
    import {onMount} from "svelte";
    import {faCirclePlay} from '@fortawesome/free-regular-svg-icons'

    type ComponentState = {
        jsonEditorRef: JSONEditor | undefined;
        optimizer: SqsgenOptimizer | undefined,
        inputConfig: any | undefined
        optimizationConfig: any | undefined
        optimizationControlButton: MenuButton | undefined
    }
    const initial = {
        structure: {
            species: ['Fe', 'Fe'],
            supercell: [3, 3, 3],
            lattice: [
                [
                    2.86,
                    0.0,
                    0.0
                ],
                [
                    0.0,
                    2.86,
                    0.0
                ],
                [
                    0.0,
                    0.0,
                    2.86
                ]
            ],
            coords: [
                [
                    0.0,
                    0.0,
                    0.0
                ],
                [
                    0.5,
                    0.5,
                    0.5
                ]
            ]
        },
        composition: [
            {
                Fe: 45,
                Al: 9
            }
        ],
        target_objective: 0
    };


    let state = $state({
        optimizer: undefined,
        inputConfig: undefined,
        optimizationControlButton: undefined,
        jsonEditorRef: undefined
    } as ComponentState);
    let content = {
        json: {
            structure: {
                species: ['Fe', 'Fe'],
                supercell: [3, 3, 3],
                lattice: [
                    [
                        2.86,
                        0.0,
                        0.0
                    ],
                    [
                        0.0,
                        2.86,
                        0.0
                    ],
                    [
                        0.0,
                        0.0,
                        2.86
                    ]
                ],
                coords: [
                    [
                        0.0,
                        0.0,
                        0.0
                    ],
                    [
                        0.5,
                        0.5,
                        0.5
                    ]
                ]
            },
            composition: [
                {
                    Fe: 45,
                    Al: 9
                }
            ],
            target_objective: 0
        }
    } as Content;


    onMount(async () => {
        state.optimizer = await loadOptimizer();
    })

    const loaded = $derived(state.optimizer !== undefined);


    function handleRenderMenu(items: MenuItem[], context: RenderMenuContext): MenuItem[] | undefined {
        const separator: MenuSeparator = {
            type: 'separator'
        }

        state.optimizationControlButton = {
            type: 'button',
            onClick: () => {
            },
            icon: faCirclePlay,
            title: 'Copy document to clipboard',
            className: 'custom-copy-button',
            disabled: state.optimizationConfig === undefined,
        };

        const head = items.slice(0, items.length - 1)
        const tail = items.slice(items.length - 1) // the tail contains space

        return head.concat(separator, state.optimizationControlButton, tail)
    }

    function handleChange(c: TextContent, _: Content, status: OnChangeStatus) {
        state.inputConfig = status.contentErrors ? undefined : JSON.parse(c.text);
        if (state.optimizer) {
            const parsedConfig = state.optimizer.parseConfig(state.inputConfig);
            state.optimizationConfig = parsedConfig.ok ? parsedConfig.value : undefined;
        }

    }

    function validator(json: any): ValidationError[] {
        return state.optimizer ? state.optimizer.validate(json) : []
    }

    $inspect(state.inputConfig)


    $effect(() => {
        if (state.optimizationControlButton) {
            state.optimizationControlButton.disabled = state.optimizationConfig === undefined;
            state.jsonEditorRef?.refresh().then(()=> {
                console.log("$effect", "refreshed", new Date());
            })
            console.log("$effect", state.optimizationControlButton.disabled, new Date());
        }

    })


</script>

{#if loaded}
    <div class="editor">
        <JSONEditor bind:this={state.jsonEditorRef} mode={Mode.text} bind:content={content} onChange={handleChange}
                    onRenderMenu={handleRenderMenu} {validator}
        />
    </div>
{/if}


<style>
    .editor {
        width: 700px;
        height: 400px;
    }
</style>
