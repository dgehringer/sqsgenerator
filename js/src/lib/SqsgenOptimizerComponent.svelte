<script lang="ts">
    import {
        type Content,
        type JSONContent,
        JSONEditor,
        type TextContent,
        Mode,
        type MenuButton, type MenuSeparator, type MenuItem, type RenderMenuContext
    } from 'svelte-jsoneditor'
    import {loadOptimizer, SqsgenOptimizer} from '$lib/optimizer.js'
    import {onMount} from "svelte";
    import {faCopy} from '@fortawesome/free-regular-svg-icons'

    type ComponentState = {
        optimizer: SqsgenOptimizer | undefined,
        content: JSONContent
    }

    let state = $state({
        optimizer: undefined,
        content: {
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
        }
    } as ComponentState)

    onMount(async () => {
        state.optimizer = await loadOptimizer();
    })

    const loaded = $derived(state.optimizer !== undefined);
    const content = $derived(state.content);
    const config = $derived(content.json);
    const runConfig = $derived(
        state.optimizer && config ? state.optimizer.parseConfig(config) : undefined
    );
    const runConfigContent = $derived(
        runConfig ? {json: runConfig.value} : undefined
    );

    function handleChange(updatedContent: TextContent) {
        try {
            state.content.json = JSON.parse(updatedContent.text);
        } catch (e) {
            state.content.json = undefined;
        }
    }

    function handleTriggerOptimization() {

    }

    function handleRenderMenu(items: MenuItem[], context: RenderMenuContext): MenuItem[] | undefined {
        console.log('handleRenderMenu', {items, context})

        const separator: MenuSeparator = {
            type: 'separator'
        }

        const triggerOptimizationButton: MenuButton = {
            type: 'button',
            onClick: handleTriggerOptimization,
            icon: faCopy,
            title: 'Copy document to clipboard',
            className: 'custom-copy-button',
            disabled: runConfig ? !runConfig.ok : true,
        }

        const head = items.slice(0, items.length - 1)
        const tail = items.slice(items.length - 1) // the tail contains space

        return head.concat(separator, triggerOptimizationButton, tail)
    }

</script>

{#if loaded}
    <div class="editor">
        <JSONEditor mode={Mode.text} {content} onChange={handleChange}
                    validator={() => state.optimizer?.validate(config)}
        onRenderMenu={handleRenderMenu}
        />
    </div>
{/if}

{#if runConfig}
    <div class="editor">
        <JSONEditor mode={Mode.text} content={runConfigContent}/>
    </div>
{/if}

<style>
    .editor {
        width: 700px;
        height: 400px;
    }
</style>
