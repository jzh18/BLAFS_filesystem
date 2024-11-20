package main

import (
	"os"
	"testing"

	"github.com/docker/docker/api/types"
	"github.com/stretchr/testify/assert"
)

func TestGenerateChainId(t *testing.T) {
	pre_chain_id := "sha256:bf1860d9c025e8eda07260833dd319960cbeeed15e87d2b9463282419b843737"
	diff_id := "sha256:d0e99c871320182afc90f346ff34eba8e96046504aff8c34e9e63ba4453bc78e"

	chain_id := generate_chainid(pre_chain_id, diff_id)

	assert.Equal(t, "25edf32f5ffff6c9eb0a4851956de2f6594ec1175d028ce2841175918f58f95c", chain_id)
}

func TestUpdateLayerSize(t *testing.T) {
	// img_info := types.ImageInspect{
	// 	RootFS: types.RootFS{
	// 		Layers: []string{
	// 			"sha256:bf1860d9c025e8eda07260833dd319960cbeeed15e87d2b9463282419b843737",
	// 			"sha256:d0e99c871320182afc90f346ff34eba8e96046504aff8c34e9e63ba4453bc78e",
	// 			"sha256:3006db7fc2e332f39a55662ea1e97726a20085975a43ba6f279533f3874ba9e5",
	// 		},
	// 	},
	// }

	// update_layer_size(&img_info, "/var/lib/docker")

}

func TestGetDirSize(t *testing.T) {
	os.Mkdir("/tmp/test_dir_size", 0755)
	os.Create("/tmp/test_dir_size/test")

	size := get_dir_size("/tmp/test_dir_size")

	assert.Equal(t, "4096", size)
}

func TestGetTopLayer(t *testing.T) {
	img_info := types.ImageInspect{
		RootFS: types.RootFS{
			Layers: []string{
				"sha256:f43c9837ee26abf2892f356bf3482d19394dc0e517be2da8682a971fcc195088",
				"sha256:fd2b0bf18351146d1333edeac5c5ada53b84fa0ef4d334158d2a7efe38cab3dc",
				"sha256:e2080d00eb62f0b7e6a5f3e8e25ff69929ddc735a79076d420ae381ccc80d7b9",
			},
		},
	}

	top_layer_sha := get_top_layer(&img_info)

	assert.Equal(t, top_layer_sha, "sha256:32274951b3ccc8634312020957980fd521cc3ddc51cde94cf4e55498fdfe0f1c")
}
