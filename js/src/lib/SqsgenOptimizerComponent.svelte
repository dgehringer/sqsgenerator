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
    } from 'svelte-jsoneditor';
    import Shepherd from 'shepherd.js';
    import 'shepherd.js/dist/css/shepherd.css';
    import { downloadAsFile, decompressData, defaultConfig, compressData } from '$lib/utils.js';
    import { Pane, Splitpanes } from 'svelte-splitpanes';
    import LinearProgress from '@smui/linear-progress';
    import CircularProgress from '@smui/circular-progress';
    import { loadOptimizer, SqsgenOptimizer } from '$lib/optimizer.js';
    import { onMount } from 'svelte';

    import {
        faCirclePlay,
        faCircleStop,
        faArrowAltCircleDown,
        faSquareCaretRight,
        faSquareCaretLeft,
        faSquareCaretDown,
        faSquareCaretUp,
        faFile,
        faCircleQuestion,
        faShareSquare
    } from '@fortawesome/free-regular-svg-icons';
    import Dialog, { Title, Content as DialogContent, Actions, InitialFocus } from '@smui/dialog';
    import Button, { Label, Icon } from '@smui/button';
    import List, { Item, Graphic, Text } from '@smui/list';
    import Radio from '@smui/radio';
    import * as Ngl from 'ngl';

    type OptimizationState = {
        is: 'idling' | 'running';
        result?: any;
        finished?: number;
        working?: number;
        stopRequested?: boolean;
    };

    type ResultViewerState = {
        objectiveIndex: number;
        structureIndex: number;
    };

    type ComponentState = {
        jsonEditorRef: JSONEditor | undefined;
        optimizer?: SqsgenOptimizer;
        inputConfig: any | undefined;
        optimizationConfig: any | undefined;
        optimization: OptimizationState;
        ngl?: any;
        viewer?: ResultViewerState;
        component?: any;
    };

    let state = $state({
        optimization: {
            is: 'idling'
        }
    } as ComponentState);
    let content = $state({
        json: defaultConfig()
    } as Content);

    onMount(async () => {
        state.optimizer = await loadOptimizer();
        state.ngl = await import('ngl');

        const params = new URLSearchParams(window.location.search);
        const configData = params.get('config');

        // Ask opener for config
        if (configData) {
            content = {
                json: await decompressData(configData)
            };
        }
        handleChange(
            { text: JSON.stringify(content.json) },
            state.jsonEditorRef?.get(),
            {} as OnChangeStatus
        );

        if (!params.has('noTour')) startTour();
    });

    let openDialogFileType = $state(false);
    let fileTypeDownload = $state('cif');
    let multiThreadingAvailable = typeof SharedArrayBuffer !== 'undefined';
    const loaded = $derived(state.optimizer !== undefined);
    const running = $derived(state.optimization.is === 'running');
    const idling = $derived(state.optimization.is === 'idling');
    const result = $derived(state.optimization.result);
    const stage = $derived(
        state.ngl
            ? new Ngl.Stage('viewport', {
                    backgroundColor: 'white',
                    cameraType: 'orthographic'
                })
            : undefined
    );

    function startTour() {
        const tour = new Shepherd.Tour({
            useModalOverlay: true,

            defaultStepOptions: {
                classes: 'shepherd-theme-custom',
                scrollTo: { behavior: 'smooth', block: 'center' },
                cancelIcon: { enabled: true }
            }
        });

        tour.addStep({
            id: 'play',
            text: 'Play/Pause the optimization',
            attachTo: { element: '.jse-button[title="Start optimization"]', on: 'bottom' },
            buttons: [
                {
                    text: 'Next',
                    action: () => {
                        runOptimization();
                        tour.next();
                    }
                }
            ]
        });

        tour.addStep({
            id: 'share',
            text: 'Create a shareable link, and share your settings.',
            attachTo: { element: '.jse-button[title="Share current configuration"]', on: 'bottom' },
            buttons: [
                { text: 'Back', action: tour.back },
                { text: 'Next', action: tour.next }
            ]
        });

        tour.addStep({
            id: 'documentation',
            text: 'Open the documentation.',
            attachTo: { element: '.jse-button[title="Open help"]', on: 'bottom' },
            buttons: [
                { text: 'Back', action: tour.back },
                { text: 'Next', action: tour.next }
            ]
        });

        tour.addStep({
            id: 'next',
            text: 'Navigate results. Switch to results with next higher objective',
            attachTo: { element: '.jse-button[title="Next objective"]', on: 'bottom' },
            buttons: [
                { text: 'Back', action: tour.back },
                { text: 'Next', action: tour.next }
            ]
        });

        tour.addStep({
            id: 'download',
            text: 'Download all results in .msgpack format of offline analysis',
            attachTo: {
                element: '.jse-button[title="Download results in msgpack format"]',
                on: 'bottom'
            },
            buttons: [
                { text: 'Back', action: tour.back },
                { text: 'Next', action: tour.next }
            ]
        });

        tour.addStep({
            id: 'download-selected',
            text: 'Download the current structure in selected format',
            attachTo: {
                element: '.jse-button[title="Download the current file in selected format"]',
                on: 'bottom'
            },
            buttons: [
                { text: 'Back', action: tour.back },
                { text: 'Next', action: tour.next }
            ]
        });

        tour.addStep({
            id: 'download-selected',
            text: 'Change file type for selected structure',
            attachTo: { element: '.jse-button[title="Change file type"]', on: 'bottom' },
            buttons: [
                { text: 'Back', action: tour.back },
                { text: 'Next', action: tour.next }
            ]
        });

        tour.addStep({
            id: 'inspect',
            text: 'Inspect selected structure',
            attachTo: { element: '#viewport', on: 'left' },
            buttons: [
                { text: 'Back', action: tour.back },
                { text: 'Done', action: tour.complete }
            ]
        });

        tour.start();
    }

    function refreshEditor() {
        if (state.jsonEditorRef) {
            state.jsonEditorRef.updateProps({
                onRenderMenu: (items: MenuItem[], context: RenderMenuContext) => {
                    return handleRenderMenu(items, context);
                }
            });
        }
        if (state.viewer) updateStage(state.viewer);
    }

    function runOptimization() {
        if (state.optimizationConfig && idling && state.optimizer) {
            state.optimization = {
                is: 'running',
                finished: 0.0,
                working: 0.0,
                stopRequested: false,
                result: undefined
            };
            state.viewer = undefined;
            const iterations = state.optimizationConfig.iterations;
            updateStage(undefined);
            state.optimizer
                .optimizeAsync(state.optimizationConfig, 0, (stats) => {
                    const finished = stats.finished / iterations;
                    if (finished > (state.optimization.finished ?? 0.0))
                        state.optimization.finished = finished;
                    state.optimization.working = stats.working / iterations;
                    return state.optimization.stopRequested;
                })
                .then((result) => {
                    state.optimization = {
                        is: 'idling',
                        result: result
                    };
                    console.log('Optimization finished:', result);
                    state.viewer = {
                        objectiveIndex: 0,
                        structureIndex: 0
                    };
                    refreshEditor();
                })
                .catch((e) => {
                    console.error('Optimization failed:', e);
                    state.optimization = {
                        is: 'idling',
                        result: undefined
                    };
                    refreshEditor();
                });
        } else if (running) state.optimization.stopRequested = true;

        refreshEditor();
    }

    function handleRenderMenu(items: MenuItem[], _: RenderMenuContext): MenuItem[] | undefined {
        const separator: MenuSeparator = {
            type: 'separator'
        };

        const buttons: MenuItem[] = [
            {
                type: 'button',
                onClick: runOptimization,
                icon: running ? faCircleStop : faCirclePlay,
                title: running ? 'Stop optimization' : 'Start optimization',
                disabled: state.optimizationConfig === undefined
            },
            {
                type: 'button',
                icon: faShareSquare,
                title: 'Share current configuration',
                onClick: () => {
                    if (!state.inputConfig) return;
                    compressData(state.inputConfig).then((compressed) => {
                        const shareUrl = `${window.location.origin}${window.location.pathname}?config=${encodeURIComponent(compressed)}`;
                        navigator.clipboard.writeText(shareUrl).then(() => {
                            alert('Shareable link copied to clipboard!');
                        });
                    });
                },
                disabled: state.optimizationConfig === undefined
            },
            {
                type: 'button',
                onClick: () => {
                    window.open('https://sqsgenerator.readthedocs.io/en/latest/parameters.html', '_blank');
                },
                icon: faCircleQuestion,
                title: 'Open help'
            }
        ];
        const disableNavButtons = result === undefined || state.viewer === undefined;
        let fileTypeButtonText = undefined;
        switch (fileTypeDownload) {
            case 'cif':
                fileTypeButtonText = 'CIF';
                break;
            case 'vasp':
                fileTypeButtonText = 'VASP';
                break;
            case 'pdb':
                fileTypeButtonText = 'PDB';
                break;
            case 'json-sqsgen':
                fileTypeButtonText = 'sqsgen';
                break;
            case 'json-ase':
                fileTypeButtonText = 'ase';
                break;
            case 'json-pymatgen':
                fileTypeButtonText = 'pymat';
                break;
            default:
                fileTypeButtonText = '???';
        }
        buttons.push(
            separator,
            {
                type: 'button',
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.objectiveIndex -= 1;
                        state.viewer.structureIndex = 0;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretDown,
                title: 'Previous objective',
                disabled: disableNavButtons || state.viewer.objectiveIndex === 0
            },
            {
                type: 'button',
                onClick: () => {},
                title: state.viewer
                    ? 'Current objective: ' + result.objective(state.viewer.objectiveIndex)
                    : undefined,
                text:
                    state.viewer && result
                        ? state.viewer.objectiveIndex + 1 + '/' + result.numObjectives()
                        : undefined
            },
            {
                type: 'button',
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.objectiveIndex += 1;
                        state.viewer.structureIndex = 0;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretUp,
                title: 'Next objective',
                disabled: disableNavButtons || state.viewer.objectiveIndex === result.numObjectives() - 1
            },
            {
                type: 'button',
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.structureIndex -= 1;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretLeft,
                title: 'Previous solution',
                disabled: disableNavButtons || state.viewer.structureIndex === 0
            },
            {
                type: 'button',
                onClick: () => {},
                text:
                    state.viewer && result
                        ? state.viewer.structureIndex +
                            1 +
                            '/' +
                            result.numSolutions(state.viewer.objectiveIndex)
                        : undefined
            },
            {
                type: 'button',
                onClick: () => {
                    if (state.viewer) {
                        state.viewer.structureIndex += 1;
                        refreshEditor();
                    }
                },
                icon: faSquareCaretRight,
                title: 'Previous solution',
                disabled:
                    result === undefined ||
                    state.viewer === undefined ||
                    state.viewer.structureIndex === result.numSolutions(state.viewer.objectiveIndex) - 1
            },
            separator,
            {
                type: 'button',
                onClick: () => {
                    downloadAsFile(new Uint8Array(result.msgpack()), 'sqs.mpack', 'application/vnd.msgpack');
                },
                disabled: result === undefined,
                icon: faArrowAltCircleDown,
                title: 'Download results in msgpack format'
            },
            {
                type: 'button',
                onClick: () => {
                    if (!result || !state.viewer) return;
                    const stem = `sqsgen-${state.viewer.objectiveIndex}-${state.viewer.structureIndex}`;
                    if (fileTypeDownload == 'cif') {
                        downloadAsFile(
                            result.cif(state.viewer.objectiveIndex, state.viewer.structureIndex),
                            `${stem}.cif`,
                            'chemical/x-cif'
                        );
                    } else if (fileTypeDownload === 'vasp') {
                        downloadAsFile(
                            result.poscar(state.viewer.objectiveIndex, state.viewer.structureIndex),
                            `${stem}.vasp`,
                            'text/plain'
                        );
                    } else if (fileTypeDownload === 'pdb') {
                        downloadAsFile(
                            result.pdb(state.viewer.objectiveIndex, state.viewer.structureIndex),
                            `${stem}.pdb`,
                            'text/plain'
                        );
                    } else if (fileTypeDownload === 'json-sqsgen') {
                        downloadAsFile(
                            result.sqsgen(state.viewer.objectiveIndex, state.viewer.structureIndex),
                            `${stem}.json`,
                            'application/json'
                        );
                    } else if (fileTypeDownload === 'json-ase') {
                        downloadAsFile(
                            result.ase(state.viewer.objectiveIndex, state.viewer.structureIndex),
                            `${stem}-ase.json`,
                            'application/json'
                        );
                    } else if (fileTypeDownload === 'json-pymatgen') {
                        downloadAsFile(
                            result.pymatgen(state.viewer.objectiveIndex, state.viewer.structureIndex),
                            `${stem}-pymat.json`,
                            'application/json'
                        );
                    }
                },
                disabled: result === undefined,
                icon: faFile,
                title: 'Download the current file in selected format'
            },
            {
                type: 'button',
                text: fileTypeButtonText,
                onClick: () => {
                    openDialogFileType = true;
                },
                className: 'filetype-label-button',
                title: 'Change file type'
            }
        );
        const space = {
            type: 'space'
        };
        const [textButton, treeButon, ...others] = items;
        treeButon.className = 'jse-group-button jse-last';

        const jseHead = [
            textButton,
            treeButon,
            ...others.slice(7, others.length - 1).filter((i) => i.type === 'button')
        ];
        console.log();

        return [...jseHead, separator, ...buttons, space];
    }

    function handleChange(c: TextContent, _: Content, status: OnChangeStatus) {
        state.inputConfig = status.contentErrors ? undefined : JSON.parse(c.text);
        if (state.optimizer) {
            const parsedConfig = state.optimizer.parseConfig(state.inputConfig);
            state.optimizationConfig = parsedConfig.ok ? parsedConfig.value : undefined;
        }

        state.jsonEditorRef?.updateProps({
            onRenderMenu: (items: MenuItem[], context: RenderMenuContext) => {
                return handleRenderMenu(items, context);
            }
        });
    }

    function validator(json: any): ValidationError[] {
        return state.optimizer ? state.optimizer.validate(json) : [];
    }

    async function updateStage(viewerState?: ResultViewerState) {
        if (!stage || !result || !viewerState) return;
        stage.removeAllComponents();
        // 2. Prepare the PDB as a blob
        const pdbText = result.pdb(viewerState.objectiveIndex, viewerState.structureIndex);
        const blob = new Blob([pdbText], { type: 'text/plain' });
        const c = await stage.loadFile(blob, { ext: 'pdb' });
        c.addRepresentation('spacefill', {
            radius_type: 'vdw',
            color_scheme: 'element',
            radius: 0.5
        });
        c.addRepresentation('unitcell');
        c.autoView();
        c.updateRepresentations({ what: { position: true, color: true } });
        stage.viewer.requestRender();
    }

    function closeHandler() {
        refreshEditor();
    }
</script>

<Dialog
    bind:open={openDialogFileType}
    selection
    aria-labelledby="list-selection-title"
    aria-describedby="list-selection-content"
    onSMUIDialogClosed={closeHandler}
>
    <Title id="list-selection-title">Choose download format</Title>
    <DialogContent id="list-selection-content">
        <List radioList>
            <Item use={[InitialFocus]}>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="cif" />
                </Graphic>
                <Text>CIF</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="vasp" />
                </Graphic>
                <Text>VASP (POSCAR)</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="pdb" />
                </Graphic>
                <Text>PDB</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="json-sqsgen" />
                </Graphic>
                <Text>JSON (sqsgen)</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="json-ase" />
                </Graphic>
                <Text>JSON (ase)</Text>
            </Item>
            <Item>
                <Graphic>
                    <Radio bind:group={fileTypeDownload} value="json-pymatgen" />
                </Graphic>
                <Text>JSON (pymatgen)</Text>
            </Item>
        </List>
    </DialogContent>
    <Actions>
        <Button action="accept">
            <Label>OK</Label>
        </Button>
    </Actions>
</Dialog>

{#if !loaded}
    <div class="loading-pane">
        <div style="display: flex; align-items: center; gap: 16px;">
            <img src="/logo_large.svg" style="width: 175px; height: auto" alt="sqsgen logo" />
            <CircularProgress style="height: 32px; width: 32px;" indeterminate />
        </div>
    </div>
{/if}

{#if loaded && multiThreadingAvailable}
    {#if running}
        <LinearProgress progress={state.optimization.finished} buffer={state.optimization.working} />
    {/if}
    <Splitpanes vertical={true} style="height: 98vh">
        <Pane minSize={30}>
            <JSONEditor
                bind:this={state.jsonEditorRef}
                mode={Mode.text}
                bind:content
                onChange={handleChange}
                onRenderMenu={handleRenderMenu}
                {validator}
            />
        </Pane>
        <Pane size={55} hidden={result === undefined}>
            <div id="viewport" style="height: 100%; width: 100%"></div>
        </Pane>
    </Splitpanes>
{/if}

<style lang="scss">
    .loading-pane {
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        width: 100vw;
    }

    :global(.shepherd-element) {
        background: var(--jse-panel-background, #fff);
        color: var(--jse-text-color, #4d4d4d);
        font-family: var(--jse-font-family, sans-serif);
        font-size: var(--jse-font-size, 16px);
        border: 1px solid var(--jse-main-border, #d7d7d7);
        border-radius: 3px;
        box-shadow: 0 2px 6px rgba(0, 0, 0, 0.15);
    }

    :global(.shepherd-text) {
        color: var(--jse-text-color, #4d4d4d);
        padding: 12px 16px;
    }

    /* the little arrow */
    :global(.shepherd-arrow::before) {
        background: var(--jse-panel-background, #fff);
        border: 1px solid var(--jse-main-border, #d7d7d7);
    }

    :global(.shepherd-header) {
        background: var(--jse-panel-background, #fff);
        padding: 10px 16px 0;
    }

    :global(.shepherd-footer) {
        padding: 0 16px 12px;
    }
</style>
