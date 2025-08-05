<script lang="ts">
    import {
        type Content,
        JSONEditor,
        type TextContent,
        Mode,
        type MenuSeparator,
        type MenuItem,
        type RenderMenuContext,
        type OnChangeStatus,
        type ValidationError
    } from 'svelte-jsoneditor'
    import LinearProgress from '@smui/linear-progress';
    import {loadOptimizer, SqsgenOptimizer} from '$lib/optimizer.js'
    import {onMount} from "svelte";
    import {faCirclePlay, faCircleStop} from '@fortawesome/free-regular-svg-icons'

    type OptimizationState = {
        is: 'idling' | 'running'
        result?: any
        finished?: number,
        working?: number,
        stopRequested?: boolean
    }

    type ComponentState = {
        jsonEditorRef: JSONEditor | undefined;
        optimizer: SqsgenOptimizer | undefined,
        inputConfig: any | undefined
        optimizationConfig: any | undefined
        optimization: OptimizationState
    }

    let state = $state({
        optimizer: undefined,
        inputConfig: undefined,
        jsonEditorRef: undefined,
        optimization: {
            is: 'idling',
        }
    } as ComponentState);
    let content = {
        json: {
            iterations: 1000000,
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
    const running = $derived(state.optimization.is === 'running');
    const idling = $derived(state.optimization.is === 'idling');

    function refreshEditor() {
        if (state.jsonEditorRef) {
            state.jsonEditorRef.updateProps({
                onRenderMenu: (items: MenuItem[], context: RenderMenuContext) => {
                    return handleRenderMenu(items, context);
                }
            });
        }
    }

    function handleRenderMenu(items: MenuItem[], context: RenderMenuContext): MenuItem[] | undefined {

        const separator: MenuSeparator = {
            type: 'separator'
        }

        let optimizationControlButton = {
            type: 'button',
            onClick: () => {
                if (state.optimizationConfig && idling && state.optimizer) {
                    state.optimization = {
                        is: 'running',
                        finished: 0.0,
                        working: 0.0,
                        stopRequested: false,
                    }
                    const iterations = state.optimizationConfig.iterations;
                    state.optimizer.optimizeAsync(state.optimizationConfig, 0, stats => {
                        const finished = stats.finished / iterations;
                        state.optimization.finished = finished > (state.optimization.finished ?? 0.0) ? finished : state.optimization.finished;
                        state.optimization.working = stats.working / iterations;
                        return state.optimization.stopRequested;
                    }).then((result) => {
                        state.optimization = {
                            is: 'idling',
                            result: result
                        };
                        refreshEditor();
                    }).catch((e) => {
                        console.error("Optimization failed:", e);
                        state.optimization = {
                            is: 'idling',
                            result: undefined
                        };
                        refreshEditor();
                    })
                } else if (running)
                    state.optimization.stopRequested = true;

                refreshEditor();
            },
            icon: running ? faCircleStop : faCirclePlay,
            title: running ? 'Stop optimization' : 'Start optimization',
            disabled: state.optimizationConfig === undefined,
        };

        const head = items.slice(0, items.length - 1)
        const tail = items.slice(items.length - 1) // the tail contains space

        return head.concat(separator, optimizationControlButton, tail)
    }

    function handleChange(c: TextContent, _: Content, status: OnChangeStatus) {
        state.inputConfig = status.contentErrors ? undefined : JSON.parse(c.text);
        if (state.optimizer) {
            const parsedConfig = state.optimizer.parseConfig(state.inputConfig);
            state.optimizationConfig = parsedConfig.ok ? parsedConfig.value : undefined;
        }

        state.jsonEditorRef?.updateProps(
            {
                onRenderMenu: (items: MenuItem[], context: RenderMenuContext) => {
                    console.log("rerendering menu");
                    return handleRenderMenu(items, context);
                }
            }
        );
    }

    function validator(json: any): ValidationError[] {
        return state.optimizer ? state.optimizer.validate(json) : []
    }


</script>

{#if loaded}


    <div class="editor">
        {#if running}
            <LinearProgress progress={state.optimization.finished} buffer={state.optimization.working}/>
        {/if}
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
