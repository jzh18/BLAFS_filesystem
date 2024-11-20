package main

import (
	"context"
	"crypto/sha256"
	"errors"
	"flag"
	"fmt"
	"io/fs"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/client"
)

/*
Create an overlay layer.

{overlay_root_dir}/

	{layer_name}/
		diff/
		link: l/link_{layer_name}
		lower
		real/
	l/
		link_{layer_name} -> ../{layer_name}/diff
*/
func create_layer(overlay_root_dir string, layer_name string, is_bottom bool) (bool, string, string, string, string, string) {
	var mode fs.FileMode = 0755
	absolute_layer_path := filepath.Join(overlay_root_dir, layer_name)
	absolute_diff_path := filepath.Join(absolute_layer_path, "diff")
	absolute_link_path := filepath.Join(absolute_layer_path, "link")
	absolute_lower_path := ""
	if !is_bottom {
		absolute_lower_path = filepath.Join(absolute_layer_path, "lower")
	}
	absolute_real_path := filepath.Join(absolute_layer_path, "real")
	absolute_l_link_path := filepath.Join(overlay_root_dir, "l", "link_"+layer_name)
	newly_create := true
	if err := os.Mkdir(absolute_layer_path, mode); err != nil {
		if errors.Is(err, os.ErrExist) {
			fmt.Println("Layer dir already exists")
			newly_create = false
			return newly_create, absolute_layer_path, absolute_diff_path, absolute_link_path, absolute_lower_path, absolute_real_path
		} else {
			panic(err)
		}
	}
	// create diff dir
	if err := os.Mkdir(absolute_diff_path, mode); err != nil {
		if errors.Is(err, os.ErrExist) {
			fmt.Println("Layer diff dir already exists")
		} else {
			panic(err)
		}
	}
	//create link file
	link_file, err := os.Create(absolute_link_path)
	if err != nil {
		if errors.Is(err, os.ErrExist) {
			fmt.Println("Layer link file already exists")
		} else {
			panic(err)
		}
	}

	// create lower dir, optionally
	if !is_bottom {
		if _, err := os.Create(absolute_lower_path); err != nil {
			if errors.Is(err, os.ErrExist) {
				fmt.Println("Layer lower file already exists")
			} else {
				panic(err)
			}
		}
	}

	// create real dir
	if err := os.Mkdir(absolute_real_path, mode); err != nil {
		if errors.Is(err, os.ErrExist) {
			fmt.Println("Layer real dir already exists")
		} else {
			panic(err)
		}
	}

	//create l/link_{layer_name}
	if err := os.Mkdir(filepath.Join(overlay_root_dir, "l"), mode); err != nil {
		if errors.Is(err, os.ErrExist) {
			fmt.Println("Link l/ dir already exists")
		} else {
			panic(err)
		}
	}
	if err := os.Symlink(filepath.Join("../", layer_name, "diff"), absolute_l_link_path); err != nil {
		if errors.Is(err, os.ErrExist) {
			fmt.Println("l/link file already exists")
		} else {
			panic(err)
		}
	}

	// write l/link_{layer_name} to the link file
	link_file.WriteString("link_" + layer_name)

	return newly_create, absolute_layer_path, absolute_diff_path, absolute_link_path, absolute_lower_path, absolute_real_path
}

func append_lower(link string, overlay_dir string, appended_link string) {
	real_path, err := os.Readlink(filepath.Join(overlay_dir, link))
	if err != nil {
		panic(err)
	}
	real_path = strings.Split(real_path, "/")[1]
	lower_path := filepath.Join(overlay_dir, real_path, "lower")
	fmt.Println(lower_path)

	f, err := os.OpenFile(lower_path, os.O_APPEND|os.O_WRONLY|os.O_CREATE, 0600)
	if errors.Is(err, os.ErrNotExist) {
		if f, err = os.Create(lower_path); err != nil {
			panic(err)
		}
		if _, err = f.WriteString(appended_link); err != nil {
			panic(err)
		}
		return
	}
	if _, err = f.WriteString(":" + appended_link); err != nil {
		panic(err)
	}

}

// the order of upper_layers matters
// insertion above an lower_layer only require one layer
func insert_layer(insert_layer string, upper_layers []string, lower_layer string) {
	fmt.Println("insert dir")
	if (upper_layers != nil && lower_layer != "") || (len(upper_layers) > 0 && lower_layer != "") {
		panic("Inserting into middle is not supported")
	}
	insert_link_data, err := ioutil.ReadFile(filepath.Join(insert_layer, "link"))
	if err != nil {
		panic(err)
	}
	link_content := string(insert_link_data)

	for _, up := range upper_layers {
		lower_path := filepath.Join(up, "lower")
		fmt.Println(lower_path)
		f, err := os.OpenFile(lower_path, os.O_APPEND|os.O_WRONLY, 0600)
		if errors.Is(err, os.ErrNotExist) {
			if f, err = os.Create(lower_path); err != nil {
				panic(err)
			}
			if _, err = f.WriteString("l/" + link_content); err != nil {
				panic(err)
			}
		} else if err == nil {
			if _, err = f.WriteString(":l/" + link_content); err != nil {
				panic(err)
			}
		} else {
			panic(err)
		}
	}

	if lower_layer != "" {
		lower_data, err := ioutil.ReadFile(filepath.Join(lower_layer, "lower"))
		if err != nil && !errors.Is(err, os.ErrNotExist) {
			panic(err)
		}
		lower_content := string(lower_data)

		lower_layer_link_data, err := ioutil.ReadFile(filepath.Join(lower_layer, "link"))
		if err != nil {
			panic(err)
		}
		lower_layer_link_content := "l/" + string(lower_layer_link_data)

		var insert_content string
		if lower_content == "" {
			insert_content = lower_layer_link_content
		} else {
			insert_content = lower_layer_link_content + ":" + lower_content
		}

		fmt.Println(link_content)
		fmt.Println(lower_content)
		insert_lower_path := filepath.Join(insert_layer, "lower")
		fmt.Println(insert_lower_path)
		fmt.Println(insert_content)
		if err := os.WriteFile(insert_lower_path, []byte(insert_content), 0600); err != nil {
			panic(err)
		}

	}

}

// sudo cached_fs -s -d  --realdir=/tmp/real3 --fileregistry=file_registry.txt /tmp/mnt
func mount_cachedfs(real_dir string, mount_point string) {
	sudo := "sudo"
	cached_fs_exe := "/home/ubuntu/repos/cached_container/build/cached_fs"
	arg0 := "-s"
	//arg1 := "-d"
	arg2 := "--realdir=" + real_dir
	arg3 := "--fileregistry=file_registry.txt"
	arg4 := mount_point
	cmd := exec.Command(sudo, cached_fs_exe, arg0, arg2, arg3, arg4)
	fmt.Println(cmd)
	stdout, err := cmd.Output()
	if err != nil {
		panic(err)
	}
	fmt.Println(string(stdout))
}

// sudo debloated_fs -s -d  --realdir=/tmp/real5 --lowerdir=/tmp/lower5  /tmp/mnt5
func mount_debloatedfs(real_dir string, mount_point string, lower_dir string) {
	sudo := "sudo"
	cached_fs_exe := "/home/ubuntu/repos/cached_container/build/debloated_fs"
	arg0 := "-s"
	arg1 := "-d"
	arg2 := "--realdir=" + real_dir
	arg3 := "--lowerdir=" + lower_dir
	arg4 := mount_point
	cmd := exec.Command(sudo, cached_fs_exe, arg0, arg1, arg2, arg3, arg4)
	fmt.Println(cmd)
	stdout, err := cmd.Output()
	if err != nil {
		panic(err)
	}
	fmt.Println(string(stdout))
}

func copy(src string, dst string) {
	// Read all content of src to data, may cause OOM for a large file.
	data, err := ioutil.ReadFile(src)
	if err != nil {
		panic(err)
	}
	// Write data to dst
	err = ioutil.WriteFile(dst, data, 0644)
	if err != nil {
		panic(err)
	}

}

// back up old cache-id and update to new cache-id
func update_cache_id(img_info *types.ImageInspect, docker_root_dir string, debloated_layer_id string) {
	// this should be calculated by: https://www.baeldung.com/linux/docker-image-storage-host
	top_layer_sha256 := strings.Split(get_top_layer(img_info), ":")[1]
	fmt.Println("top layer: ", top_layer_sha256)
	top_layer_cache_id_path := filepath.Join(docker_root_dir, "image/overlay2/layerdb/sha256", top_layer_sha256, "cache-id")
	top_layer_cache_id_bak_path := filepath.Join(docker_root_dir, "image/overlay2/layerdb/sha256", top_layer_sha256, "cache-id.bak")
	copy(top_layer_cache_id_path, top_layer_cache_id_bak_path)
	if err := os.WriteFile(top_layer_cache_id_path, []byte(debloated_layer_id), 0600); err != nil {
		panic(err)
	}
}

// path like: /var/lib/docker/overlay2/pd7ufvun8w4iczembtt928jio/diff
func truncate_layers(abs_layer_path []string) {
	for _, layer := range abs_layer_path {
		fmt.Println("Truncate layer: ", layer)
		// this will remove the diff layer too
		if err := os.RemoveAll(layer); err != nil {
			panic(err)
		}
		// create an empty diff layer
		if err := os.Mkdir(layer, 0755); err != nil {
			panic(err)
		}

	}
}

func get_dir_size(abs_path string) string {
	fmt.Println("get dir size: ", abs_path)
	cmd := exec.Command("sudo", "du", "-sb", abs_path)
	stdout, err := cmd.Output()
	if err != nil {
		panic(err)
	}
	size := strings.Fields(string(stdout))[0]

	return size
}

func update_layer_size(img_info *types.ImageInspect, docker_root_dir string) {
	rootfs_layers := img_info.RootFS.Layers
	chain_id := rootfs_layers[0]

	for _, diff_id := range rootfs_layers[1:] {
		dir_name := strings.Split(chain_id, ":")[1]
		// update layer size
		layer_dir := filepath.Join(docker_root_dir, "image/overlay2/layerdb/sha256", dir_name)
		cache_id, err := ioutil.ReadFile(filepath.Join(layer_dir, "cache-id"))
		if err != nil {
			panic(err)
		}
		cache_id_str := string(cache_id)
		abs_diff_dir := filepath.Join(docker_root_dir, "overlay2", cache_id_str, "diff")
		size := get_dir_size(abs_diff_dir)
		fmt.Println(size)
		if err := os.WriteFile(filepath.Join(layer_dir, "size"), []byte(size), 0600); err != nil {
			panic(err)
		}

		// generate new chain_id
		chain_id = generate_chainid(chain_id, diff_id)
		chain_id = "sha256:" + chain_id
	}

	// last layer is the debloated_fs layer
	chain_id = strings.Split(chain_id, ":")[1]
	layer_dir := filepath.Join(docker_root_dir, "image/overlay2/layerdb/sha256", chain_id)
	cache_id, err := ioutil.ReadFile(filepath.Join(layer_dir, "cache-id"))
	if err != nil {
		panic(err)
	}
	cache_id_str := string(cache_id)
	// use size of real dir
	abs_diff_dir := filepath.Join(docker_root_dir, "overlay2", cache_id_str, "real")
	size := get_dir_size(abs_diff_dir)
	fmt.Println(size)
	if err := os.WriteFile(filepath.Join(layer_dir, "size"), []byte(size), 0600); err != nil {
		panic(err)
	}

}

func generate_chainid(pre_chain_id string, diff_id string) string {
	str := pre_chain_id + " " + diff_id
	chain_id := fmt.Sprintf("%x", sha256.Sum256([]byte(str)))
	return chain_id
}

func restart_docker() {
	cmd := exec.Command("sudo", "systemctl", "restart", "docker")
	fmt.Println(cmd)
	stdout, err := cmd.Output()
	if err != nil {
		panic(err)
	}
	fmt.Println(string(stdout))
}

func get_top_layer(img_info *types.ImageInspect) string {
	rootfs_layers := img_info.RootFS.Layers
	chain_id := rootfs_layers[0]
	for _, diff_id := range rootfs_layers[1:] {
		chain_id = generate_chainid(chain_id, diff_id)
		chain_id = "sha256:" + chain_id
	}
	return chain_id
}

func mount_overlay(merged string, upper string, work string, lower_dirs []string) {
	lowers := ""
	for _, l := range lower_dirs {
		lowers = lowers + l + ":"
	}

	lowers = lowers[0 : len(lowers)-1]

	cmd := exec.Command("sudo", "mount", "-t", "overlay", "-o", "lowerdir="+lowers+",upperdir="+upper+",workdir="+work, "overlay", merged)
	fmt.Println("mount layer: ", cmd)
	if _, err := cmd.Output(); err != nil {
		panic(err)
	}
}
func unmount(mount_point string) {
	cmd := exec.Command("sudo", "umount", "-f", mount_point)
	fmt.Println("umount layer: ", cmd)
	cmd.Output()
}

func main() {
	// test1, _, _, _, _ := create_layer("/tmp/cached_container/", "test1", true)
	// test2, _, _, _, _ := create_layer("/tmp/cached_container/", "test2", true)
	// insert_layer(test2, []string{test1}, "")
	// insert, _, _, _, _ := create_layer("/tmp/cached_container/", "insert", true)
	// insert_layer(insert, []string{test1, test2}, "")

	// top, _, _, _, _ := create_layer("/tmp/cached_container/", "top", false)
	// insert_layer(top, nil, test1)
	cachedPtr := flag.Bool("cached", false, "a bool")
	debloatedPtr := flag.Bool("debloated", false, "a bool")
	cleanPtr := flag.Bool("clean", false, "a bool")
	image_namePtr := flag.String("name", "", "image name")
	umountPtr := flag.Bool("umount", false, "umount related layers")
	flag.Parse()
	fmt.Println("cached layer enabled: ", *cachedPtr)
	fmt.Println("debloated layer enabled: ", *debloatedPtr)
	fmt.Println("clean: ", *cleanPtr)
	fmt.Println("container name: ", *image_namePtr)
	fmt.Println("umount: ", *umountPtr)

	ctx := context.Background()
	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		panic(err)
	}
	defer cli.Close()

	docker_info, _ := cli.Info(ctx)
	docker_root_dir := docker_info.DockerRootDir
	overlay_path := filepath.Join(docker_root_dir, "overlay2")

	img_name := *image_namePtr

	if *umountPtr {
		img_info, _, err := cli.ImageInspectWithRaw(ctx, img_name)
		if err != nil {
			panic(err)
		}
		all_lowers := strings.Split(img_info.GraphDriver.Data["LowerDir"], ":")

		// umount cached layer
		absolute_cached_diff_path := all_lowers[len(all_lowers)-1]
		unmount(absolute_cached_diff_path)

		// umount original layer
		original_merged := all_lowers[0]
		original_base := original_merged[:len(original_merged)-4]
		original_merged = original_base + "merged"
		unmount(original_merged)

		// umount debloated layer
		absolute_debloated_diff_path := img_info.GraphDriver.Data["UpperDir"]
		unmount(absolute_debloated_diff_path)
	}

	// Create cached layer dir

	if *cachedPtr {
		img_info, _, err := cli.ImageInspectWithRaw(ctx, img_name)
		if err != nil {
			panic(err)
		}
		img_id := strings.Split(img_info.ID, ":")[1]

		newly_create, absoulte_cached_layer_path, absolute_cached_diff_path, _, _, absolute_cached_real_path := create_layer(overlay_path, "cached_"+img_id, true)

		img_upper_dir := img_info.GraphDriver.Data["UpperDir"]
		img_upper_dir = img_upper_dir[:len(img_upper_dir)-4]
		if !newly_create {
			fmt.Println("Already patched")
		} else {
			// append cached layer to the existing layers
			var upper_dirs []string

			upper_dirs = append(upper_dirs, img_upper_dir)
			upper_lowers_dir := filepath.Join(img_upper_dir, "lower")
			upper_lowers, err := ioutil.ReadFile(upper_lowers_dir)
			if err != nil {
				fmt.Println("no lower file, this might be the lowest layer, too.")
			}
			upper_lowers_str := string(upper_lowers)
			var lowers []string
			if upper_lowers_str == "" {
				lowers = []string{}
			} else {
				lowers = strings.Split(upper_lowers_str, ":")
			}
			fmt.Println("num of lowers: ", len(lowers))

			for _, e := range lowers {
				real_path, err := os.Readlink(filepath.Join(overlay_path, e))
				if err != nil {
					panic(err)
				}
				real_path = strings.Split(real_path, "/")[1]
				real_path = filepath.Join(overlay_path, real_path)
				upper_dirs = append(upper_dirs, real_path)
			}

			insert_layer(absoulte_cached_layer_path, upper_dirs, "")
		}

		// mount cached_fs
		mount_cachedfs(absolute_cached_real_path, absolute_cached_diff_path)
	}

	// Create debloated layer
	if *debloatedPtr {
		img_info, _, err := cli.ImageInspectWithRaw(ctx, img_name)
		if err != nil {
			panic(err)
		}
		img_id := strings.Split(img_info.ID, ":")[1]

		img_upper_dir := img_info.GraphDriver.Data["UpperDir"]
		img_upper_dir = img_upper_dir[:len(img_upper_dir)-4]

		debloated_layer_name := "debloated_" + img_id
		newly_create, absoulte_debloated_layer_path, absolute_debloated_diff_path, _, _, absolute_debloated_real_path := create_layer(overlay_path, debloated_layer_name, false)

		var all_lowers []string
		var original_merged string
		var original_work string
		var original_upper string
		if !newly_create {
			fmt.Println("already patched debloated layer")
		} else {
			insert_layer(absoulte_debloated_layer_path, nil, img_upper_dir)
			// update cache-id to make the debloated layer visible to docker service
			// check out another way to update cache_id: modify img config file (not a good option)
			update_cache_id(&img_info, docker_root_dir, debloated_layer_name)
			restart_docker()
			img_info, _, err = cli.ImageInspectWithRaw(ctx, img_name)
			if err != nil {
				panic(err)
			}
		}

		lowers := strings.Split(img_info.GraphDriver.Data["LowerDir"], ":")
		fmt.Println("lowers: ", lowers)
		if *cachedPtr {
			lowers = lowers[0 : len(lowers)-1]
		}

		// umount before to avoid double mount crash
		all_lowers = strings.Split(img_info.GraphDriver.Data["LowerDir"], ":")
		original_upper = all_lowers[0]
		original_base := original_upper[:len(original_upper)-4]
		original_merged = original_base + "merged"
		original_work = original_base + "work"
		all_lowers = all_lowers[1:]
		os.Mkdir(original_merged, 0755)
		if len(all_lowers) > 0 {
			unmount(original_merged)
			mount_overlay(original_merged, original_upper, original_work, all_lowers)
		} else {
			// only one layer, no need to merge
			original_merged = original_base + "diff"
		}

		// remove layers and update size
		duration := 30
		go func() {
			for {
				time.Sleep(time.Duration(duration) * time.Second)
				truncate_layers(lowers)
				update_layer_size(&img_info, docker_root_dir)
				duration += 10
			}
		}()

		// mount debloated fs
		mount_debloatedfs(absolute_debloated_real_path, absolute_debloated_diff_path, original_merged)

	}

}
