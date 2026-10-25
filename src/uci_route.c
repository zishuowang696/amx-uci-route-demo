#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_object_parameter.h>
#include <amxo/amxo.h>
#include <uci.h>

static amxd_dm_t *g_dm;

static const char *param(const amxd_object_t *obj, const char *name)
{
    const amxc_var_t *value = amxd_object_get_param_value(obj, name);
    return value ? GET_CHAR(value, NULL) : NULL;
}

static int set_option(struct uci_context *ctx,
                      struct uci_package *pkg,
                      struct uci_section *section,
                      const char *name,
                      const char *value)
{
    struct uci_ptr ptr = {
        .p = pkg,
        .s = section,
        .section = section->e.name,
        .option = name,
        .value = value,
    };
    return uci_set(ctx, &ptr);
}

static int apply_route(const char *name,
                       const char *target,
                       const char *netmask,
                       const char *gateway,
                       const char *iface)
{
    struct uci_context *ctx = uci_alloc_context();
    struct uci_package *pkg = NULL;
    struct uci_section *section;
    int rc = -1;

    if (!ctx)
        return -1;

    if (getenv("UCI_CONFIG_DIR"))
        uci_set_confdir(ctx, getenv("UCI_CONFIG_DIR"));

    if (uci_load(ctx, "network", &pkg) != UCI_OK) {
        uci_perror(ctx, "uci_load network");
        goto out;
    }

    section = uci_lookup_section(ctx, pkg, name);
    if (!section && uci_add_section(ctx, pkg, "route", &section) != UCI_OK) {
        uci_perror(ctx, "uci_add_section route");
        goto out;
    }

    if (set_option(ctx, pkg, section, "interface", iface) != UCI_OK)
        goto out;
    if (set_option(ctx, pkg, section, "target", target) != UCI_OK)
        goto out;
    if (set_option(ctx, pkg, section, "netmask", netmask) != UCI_OK)
        goto out;
    if (set_option(ctx, pkg, section, "gateway", gateway) != UCI_OK)
        goto out;

    if (uci_commit(ctx, &pkg, false) != UCI_OK) {
        uci_perror(ctx, "uci_commit network");
        goto out;
    }

    rc = 0;
out:
    uci_free_context(ctx);
    return rc;
}

void route_changed(const char *event_name,
                   const amxc_var_t *event_data,
                   void *priv)
{
    const char *path = GET_CHAR(event_data, "path");
    const char *alias;
    const char *target;
    const char *netmask;
    const char *gateway;
    const char *iface;
    amxd_object_t *obj;

    (void)event_name;
    (void)priv;

    if (!g_dm || !path || strncmp(path, "UciRoute.", 9) != 0)
        return;

    obj = amxd_dm_findf(g_dm, "%s", path);
    if (!obj)
        return;

    alias = param(obj, "Alias");
    target = param(obj, "Target");
    netmask = param(obj, "Netmask");
    gateway = param(obj, "Gateway");
    iface = param(obj, "Interface");

    if (!alias || !target || !netmask || !gateway || !iface)
        return;

    if (apply_route(alias, target, netmask, gateway, iface) != 0)
        fprintf(stderr, "uci-route-demo: failed to apply %s\n", alias);
}

void _route_changed(const char *event_name,
                    const amxc_var_t *event_data,
                    void *priv)
{
    route_changed(event_name, event_data, priv);
}

int uci_route_main(int reason, amxd_dm_t *dm, amxo_parser_t *parser)
{
    (void)parser;
    if (reason == AMXO_START)
        g_dm = dm;
    return 0;
}

int _uci_route_main(int reason, amxd_dm_t *dm, amxo_parser_t *parser)
{
    return uci_route_main(reason, dm, parser);
}
